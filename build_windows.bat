gcc -Wall -Werror -O3 -o l_system_3d ^
  l_system_3d.c ^
  l_system_mesh.c ^
  turtle_3d.c ^
  parse_config.c ^
  utilities.c ^
  glad\src\glad.c ^
  -I cglm\include ^
  -I glad\include ^
  -I C:\bin\glfw-3.3.3\include ^
  -L C:\bin\glfw-3.3.3\lib-static-ucrt ^
  -lglfw3dll ^
  -lm

