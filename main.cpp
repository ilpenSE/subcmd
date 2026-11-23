#include <iostream>
#define SUBCMD_IMPL
#include "subcmd.hpp"

void print_usage(const char *program_name, FILE *f = stdout)
{
  std::fprintf(f, "Usage: %s <command> [...OPTIONS]\n", program_name);
  subcmd::print_commands(4, f);
}

// argv contains subcommand itself ("help")
bool help_handler(const char *program_name, int argc, char **argv) {
  (void)subcmd::shift(argc, argv); // skip command name

  const char *arg = subcmd::shift(argc, argv);
  if (!arg) { print_usage(program_name); return true; }

  std::string_view command_name = arg;
  if (subcmd::exists(command_name)) {
    subcmd::print_command(command_name, program_name);
    return true;
  }

  std::cerr << "ERROR: command '" << command_name << "' not found!" << std::endl;
  return false;
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
    {.name="--foo", .desc="description for foo", .usage="", .handler=foo_handler},
  });

  // Register alias(es)
  // Aliases cannot alias to each other
  // Aliases must alias to existing command
  // Alias name mustn't be an existing command
  subcmd::add_aliases({
    {.alias="-h", .to="--help"},
    {.alias="-f", .to="--foo"},
  });

  // You can also do single alias like this
  //subcmd::add_alias({.alias="-h", .to="--help"});

  // Parse command then call handler by yourself
  if (!subcmd::parse(program_name, argc, argv, [program_name](std::optional<std::string_view> cmd_name){
    if (cmd_name) {
      std::cerr << "ERROR: command '" << *cmd_name << "' not found!" << std::endl;
    } else {
      std::cerr << "ERROR: command is required!" << std::endl;
    }
    print_usage(program_name, stderr);
  }, /* you can slap a lambda here to do things on a command execution failed */)) return 1;
}
