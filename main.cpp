#include <iostream>
#define SUBCMD_IMPL
#include "subcmd.hpp"

// argv contains subcommand itself ("help")
bool help_handler(const char *program_name, int argc, char **argv) {
  (void)program_name;
  for (int i = 0; i < argc; i++) {
    std::cout << "arg [" << i << "] = " << argv[i] << std::endl;
  }
  return true;
}

bool foo_handler(const char *program_name, int argc, char **argv) {
  (void)program_name; (void)argc; (void)argv;
  std::cout << "foo" << std::endl;
  return true;
}

int main(int argc, char **argv)
{
  // First, get program name using shift.
  const char *program_name = subcmd::shift(argc, argv);

  // Register command(s)
  // Commands mustn't collide with each other
  subcmd::add_commands({
    {.name="--help", .desc="description for help", .usage="[command]", .handler=help_handler},
    {.name="foo", .desc="description for foo", .usage="", .handler=foo_handler},
  });

  // Register alias(es)
  // Aliases cannot alias to each other
  // Aliases must alias to existing command
  // Alias name mustn't be an existing command
  subcmd::add_aliases({
    {.alias="-h", .to="--help"},
    {.alias="fooa", .to="--help"},
  });

  // You can also do single alias like this
  //subcmd::add_alias({.alias="-h", .to="--help"});

  // Parse command then call handler by yourself
  auto [handler, cmd_name] = subcmd::parse(argc, argv);
  if (handler) {
    if (!handler(program_name, argc, argv)) return 1;
  } else {
    std::cerr << "ERROR: command '" << cmd_name << "' not found" << std::endl;
    return 1;
  }
}
