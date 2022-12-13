#
# DANTE :: System Automatycznych Testów Jednostkowych oraz Akceptacyjnych
# Tomasz Jaworski, 2018-2020
#
# Plik wygenerowany automatycznie
# Znacznik czasowy: 2022-12-12 19:23:04.706140
#

cmake_minimum_required(VERSION 3.17)
project(project1 C)

# Przyjmij standard C11
set(CMAKE_C_STANDARD 11)

# Ustaw opcje kompilatora (z raportu Dante)
add_compile_options(
      "-Werror"
      "-pedantic"
      "-fmax-errors=5"
      "-Wno-error=implicit-fallthrough"
      "-D_GNU_SOURCE"
      "-Wno-error=unused-parameter"
      "-DINSIDE_DANTE"
      "-Wno-parentheses"
      "-D_ANSI_OUTPUT"
      "-ggdb3"
      "-Wall"
      "-xc"
      "-Wno-error=parentheses"
      "-std=c11"
      "-Werror=vla"
      "-fdiagnostics-color"
      "-Wextra"

)

# Ustaw opcje konsolidatora (z raportu Dante)
add_link_options(
        "-Wl,-cref"
        "-Wl,-Map=main.map"
        "-ggdb3"
        "-Wl,-wrap,main"
)

# Utwórz target 'project1' i dodaj do niego pliki źródlowe:
add_executable(project1
        "main.c"
        "heap.c"
        "unit_helper_v2.c"
        "unit_test_v2.c"
        "rdebug.c"
        "memmanager.c"
)

# Dołącz niezbędne biblioteki
target_link_libraries(project1
        "pthread"
        "m"
)