
import os
import numpy as np


def save_tables(grid, properties, table_root='./tables', axes=['x', 'y', 'z', 't']):
  os.makedirs(table_root, exist_ok=True)
  for ii, d in zip(range(len(grid)), axes):
    np.savetxt('%s/%s.csv' % (table_root, d),
               grid[ii],
               fmt='%1.5f',
               delimiter=',')

  for ka in properties:
    np.savetxt('%s/%s.csv' % (table_root, ka),
               np.reshape(properties[ka], (-1), order='F'),
               fmt='%1.5e',
               delimiter=',')


def load_tables(axes, properties, table_root='./tables', extension='csv'):
  # Load axes
  X = [np.loadtxt('%s/%s.%s' % (table_root, f, extension), unpack=True) for f in axes]
  N = tuple([len(tmp) for tmp in X])

  # Load properties
  p = {f: np.reshape(np.loadtxt('%s/%s.%s' % (table_root, f, extension)), N, order='F') for f in properties}

  return X, p

