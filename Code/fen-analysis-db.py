import logging
import sys
import os
import json
import random

import time
import sqlitedict

import chess
from chess import WHITE, BLACK
import chess.engine

from absl import app
from absl import flags

from util import *

FLAGS = flags.FLAGS
flags.DEFINE_string('fen', None, 'FEN input file')
flags.DEFINE_string('output', None, 'Output sqlite')
flags.DEFINE_string('reference', None, 'Reference sqlite')
flags.DEFINE_integer('depth', 1, 'Max depth')
flags.DEFINE_bool('debug', False, '')
flags.DEFINE_bool('verbose', False, '')
flags.DEFINE_bool('overwrite', False, '')

flags.mark_flag_as_required('fen')
flags.mark_flag_as_required('output')

MULTIPV = 1

HASH = 512
THREADS = 1 # reproducible

COMMIT_FREQ_SECS = 300.0

def open_engine():
  engine = chess.engine.SimpleEngine.popen_uci('./stockfish')
  engine.configure({"Hash": HASH})
  engine.configure({"Threads": THREADS})
  return engine


def main(argv):
  del argv
  if FLAGS.debug:
    logging.basicConfig(level=logging.DEBUG)
    logging.getLogger().setLevel(logging.DEBUG)

  assert os.access(FLAGS.fen, os.R_OK)

  reference = set()
  if FLAGS.reference:
    print(f'Reference {FLAGS.reference}')
    assert os.access(FLAGS.reference, os.R_OK)
    rdb = sqlitedict.open(filename=FLAGS.reference,
                          flag='c',
                          encode=json.dumps,
                          decode=json.loads)

    reference.update(list(rdb.keys()))
    print(f'Reference {FLAGS.reference} : {len(reference)} positions/depths')
    rdb.close()
    del rdb

  print(f'Open {FLAGS.fen}')
  fens = open(FLAGS.fen, 'r').read().split('\n')
  fens = [fen.strip() for fen in fens]

  ncache = 0
  ncache_ref = 0
  nwrite = 0

  print(f'Db: {FLAGS.output}')
  db = sqlitedict.open(FLAGS.output,
                       flag='c',
                       encode=json.dumps,
                       decode=json.loads)

  reference.update(list(db.keys()))

  t_last_commit = time.time()
  max_dt = 0.0
  max_nodes = 0
  t_start = time.time()

  engine = open_engine()
  for nfen, fen in enumerate(fens):
    if len(fen) == 0:
      continue

    now = time.time()
    if now >= (t_last_commit + COMMIT_FREQ_SECS):
      t1 = time.time()
      db.commit()
      t_last_commit = now

    t1 = time.time()

    nodes = 0
    board = chess.Board(fen)

    for depth in range(1, FLAGS.depth + 1):
      sfen = f'{fen}|{depth}'
      if not FLAGS.overwrite:
        if sfen in reference:  # In memory
          ncache_ref += 1
          continue
        if sfen in db:  # Hmm, maybe should be in memory
          ncache += 1
          continue

      engine.configure({"Clear Hash": None})
      multi = engine.analyse(board, chess.engine.Limit(depth=depth), multipv=MULTIPV)

      if len(multi) > 0:
        nodes += multi[0].get('nodes', 0)

      db[sfen] = list(simplify_multi(multi, board))
      if FLAGS.verbose:
        print(db[sfen])
      nwrite += 1
    dt = time.time() - t1
    star = ''
    if dt > max_dt:
      max_dt = dt
      star += 't'
    if nodes > max_nodes:
      max_nodes = nodes
      star += '*'
    print(f'{nfen:4d} {int(100.0*nfen/len(fens))}% FEN: {fen} {dt:.1f}s {nodes} {star}')

  print('Cache: ', ncache)
  print('Cache: ', ncache_ref, '(reference db)')
  print('Write: ', nwrite)
  db.commit()
  db.close()
  engine.quit()

if __name__ == "__main__":
  app.run(main)
