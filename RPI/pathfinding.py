#!/usr/bin/python
# -*- config: utf-8 -*-
#
# PathFinding using A* algorithm
#
# ----------
# User Instructions:
# 
# Define a function, search() that takes no input
# and returns a list
# in the form of [optimal path length, x, y]. For
# the grid shown below, your function should output
# [11, 4, 5].
#
# If there is no valid path from the start point
# to the goal, your function should return the string
# 'fail'
# ----------

# Grid format:
# 0 = Navigable space
# 1 = Occupied space

grid = [[0, 0, 1, 0, 0, 0],
       [0, 0, 1, 0, 0, 0],
       [0, 0, 0, 0, 1, 0],
       [0, 0, 1, 1, 1, 0],
       [0, 0, 0, 0, 1, 0]] # this is the same as image grid

init = [0, 0] # starting position
goal = [len(grid)-1, len(grid[0])-1] # goal position
 
delta = [[-1, 0], # go up
        [0, -1], # go left
        [1, 0], # go down
        [0, 1]] # do right

delta_name = ['^', '<', 'v', '>']

# there are four motion directions: up/left/down/right
# increasing the index in this array corresponds to
# a left turn. Decreasing is is a right turn.

forward = [[-1, 0], # go up
          [ 0, -1], # go left
          [ 1, 0], # go down
          [ 0, 1]] # do right

forward_name = ['up', 'left', 'down', 'right']

cost = 1 # # the cost associated with moving from a cell to an adjacent one. each move costs 1

def search():
 closed = [[0] * len(grid[0]) for i in grid]
 closed[init[0]][init[1]] = 1
 action = [[-1] * len(grid[0]) for i in grid]
 
 x = init[0]
 y = init[1]
 g = 0
 
 open = [[g, x, y]]
 
 found = False # flag that is set when search is complet
 resign = False # flag set if we can't find expand
 
 while not found and not resign:
 if len(open) == 0:
 resign = True
 return 'fail'
 else:
 open.sort()
 open.reverse()
 next = open.pop()
 x = next[1]
 y = next[2]
 g = next[0]
 
 if x == goal[0] and y == goal[1]:
 found = True
 else:
 for i in range(len(delta)):
 x2 = x + delta[i][0]
 y2 = y + delta[i][1]
 if x2 >= 0 and x2 < len(grid) and y2 >=0 and y2 < len(grid[0]):
   if closed[x2][y2] == 0 and grid[x2][y2] == 0:
     g2 = g + cost
     open.append([g2, x2, y2])
     closed[x2][y2] = 1
     action[x2][y2] = i
     policy = [[' '] * len(grid[0]) for i in grid]
     x = goal[0]
     y = goal[1]
     policy[x][y] = '*'
     while x != init[0] or y != init[1]:
       x2 = x - delta[action[x][y]][0]
       y2 = y - delta[action[x][y]][1]
       policy[x2][y2] = delta_name[action[x][y]]
       x = x2
       y = y2
 for row in policy:
   print row
 return policy # make sure you return the shortest path.

def compute_value():
 value = [[99 for row in range(len(grid[0]))] for col in range(len(grid))]
 change = True
 while change:
 change = False
 for x in range(len(grid)):
 for y in range(len(grid[0])):
 if goal[0] == x and goal[1] == y:
 if value[x][y] > 0:
 value[x][y] = 0
 change = True
 elif grid[x][y] == 0:
 for a in range(len(delta)):
 x2 = x + delta[a][0]
 y2 = y + delta[a][1]
 if x2 >= 0 and x2 < len(grid) and y2 >=0 and y2 < len(grid[0]):
   v2 = value[x2][y2] + cost_step
 if v2 < value[x][y]:
   change = True
   value[x][y] = v2

def optimum_policy2D():
 value = [[[999 for row in range(len(grid[0]))] for col in 
range(len(grid))],
 [[999 for row in range(len(grid[0]))] for col in 
range(len(grid))],
 [[999 for row in range(len(grid[0]))] for col in 
range(len(grid))],
 [[999 for row in range(len(grid[0]))] for col in 
range(len(grid))]]
 policy = [[[' ' for row in range(len(grid[0]))] for col in 
range(len(grid))],
 [[' ' for row in range(len(grid[0]))] for col in 
range(len(grid))],
 [[' ' for row in range(len(grid[0]))] for col in 
range(len(grid))],
 [[' ' for row in range(len(grid[0]))] for col in 
range(len(grid))]]
 policy2D = [[' ' for row in range(len(grid[0]))] for col in 
range(len(grid))]
 change = True
 while change:
 change = False
 for x in range(len(grid)):
 	for y in range(len(grid[0])):
 for orientation in range(4):
 if goal[0] == x and goal[1] == y:
 if value[orientation][x][y] > 0:
 value[orientation][x][y] = 0
 policy[orientation][x][y] = '*'
 change = True
 elif grid[x][y] == 0:
 for i in range(3):
 o2 = (orientation + action[i]) % 4
 x2 = x + forward[o2][0]
 y2 = y + forward[o2][1]
 if x2 >= 0 and x2 < len(grid) and y2 >= 0 and y2 < len(grid[0]) and grid[x2][y2] == 0:
   v2 = value[o2][x2][y2] + cost[i]
 if v2 < value[orientation][x][y]:
 change = True
 value[orientation][x][y] = v2
 policy[orientation][x][y] = action_name[i] 
 x = init[0]
 y = init[1]
 orientation = init[2]
 policy2D[x][y] = policy[orientation][x][y]
 while policy[orientation][x][y] != '*':
 if policy[orientation][x][y] == '#':
 o2 = orientation
 elif policy[orientation][x][y] == 'R':
 o2 = (orientation - 1) % 4
 elif policy[orientation][x][y] == 'L':
 o2 = (orientation + 1) % 4
 x = x + forward[o2][0]
 y = y + forward[o2][1]
 orientation = o2
 policy2D[x][y] = policy[orientation][x][y]
 return policy2D # Make sure your function returns the expected grid




