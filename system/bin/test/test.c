#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mario/mario_vm.h>


const char* js = " \
	var s = \"Misa\"; \
  function hello() { \
    debug(\"Hello JS world!\" + s); \
  } \
  hello();";

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

  vm_t* vm = vm_new(compile);

  vm_init(vm, NULL, NULL); //initialize the vm enviroment.

  vm_load_run(vm, js); // load JS script (and compile to bytecode) and run

  vm_close(vm); //release
  return 0;
}

