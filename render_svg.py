def generate_ascii(points_list, width=50, height=50, max_x=500, max_y=500):
   grid = [[' ' for _ in range(width)] for _ in range(height)]
   for points in points_list:
       for x, y in points:
           grid_x = int((x / max_x) * (width - 1))
           grid_y = int((y / max_y) * (height - 1))
           if 0 <= grid_x < width and 0 <= grid_y < height:
               grid[grid_y][grid_x] = '*'
   
   print('-' * width)
   for row in grid:
       print(''.join(row))
   print('-' * width)

polygon_pts = [
  (111.3, 60.4), (467.6, 58.3), (465.6, 376.0), (450.7, 397.6),
  (382.1, 397.2), (372.4, 381.3), (373.2, 189.4), (346.9, 187.4),
  (347.7, 376.1), (331.5, 397.4), (254.0, 399.4), (243.3, 388.5)
]

polyline_pts = [
  (113.1, 194.2), (152.5, 110.4), (207.1, 220.2), (274.7, 133.1),
  (281.8, 231.6), (358.6, 170.0), (331.4, 272.8), (394.7, 349.0),
  (321.5, 343.5), (286.7, 428.4), (236.6, 343.1), (147.3, 386.1),
  (144.5, 338.0), (75.8, 303.2), (148.4, 275.3), (27.6, 198.0),
  (103.4, 205.2)
]

print("Original Points Layout:")
generate_ascii([polygon_pts, polyline_pts], width=80, height=40)
