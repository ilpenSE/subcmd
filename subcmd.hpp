/**
 * @file subcmd.hpp
 * @author ilpeN
 * @version >=C++20
*/

#ifndef SUBCMD_HPP
#define SUBCMD_HPP

#include <string_view>
#include <utility>
#include <unordered_map>
#include <cassert>
#include <source_location>
#include <format>
#include <cstdio>

#ifndef NDEBUG
#define SUBCMD_DEBUG
#endif

namespace subcmd {
using Loc = std::source_location;

using handler_fn = bool(*)(const char *program_name, int argc, char **argv);

struct Command {
  std::string_view name;
  std::string_view desc;
  std::string_view usage;
  handler_fn handler = nullptr;
  Loc defined_at;
};

struct Alias {
  std::string_view alias;
  std::string_view to;
  Loc defined_at;
};

inline std::unordered_map<std::string_view, Command> cmds;
inline std::unordered_map<std::string_view /*alias*/, Alias /*to*/> als;

/**
 * @brief Shift an element from argv
 * @param argc Count of args vector (mutates)
 * @param argv Args vector (mutates)
 * @return Shifted element if args contains at least 1 element, nullptr otherwise
*/
extern const char *shift(int &argc, char **&argv);

/**
 * @brief Registers a comand
 * @param cmd Command structure to be registered
 * @param loc Source code location to where this function called
 * If you try to register an existing command, it'll crash the program in debug mode.
 * It's YOUR responsible to NOT do multiple registration of same command.
 * It WON'T check this case in release mode.
*/
extern void add_command(Command &cmd, Loc loc = Loc::current());

/**
 * @brief Registers multiple commands (calls add_command for each element)
 * @param commands List of commands to be registered
 * @param loc Source code location to where this function called
*/
extern void add_commands(std::initializer_list<Command> commands, Loc loc = Loc::current());

/**
 * @brief Make an alias for a command
 * @param alias The alias structure to be made
 * @param loc Source code location to where this function called
 * It's YOUR responsible to NOT have multiple registrations,
 * cross-aliasing (having an alias that aliases to other alias) or
 * dangling aliasing (an alias that points to non-existing command).
 * In this cases, program'll crash in debug mode.
 * It WON'T check any of this cases in release mode.
*/
extern void add_alias(Alias alias, Loc loc = Loc::current());

/**
 * @brief Makes multiple aliases (calls add_alias for each element)
 * @param aliases List of aliases to be made
 * @param loc Source code location to where this function called
*/
extern void add_aliases(std::initializer_list<Alias> aliases, Loc loc = Loc::current());

/**
 * @brief Parse a command using args vector
 * @param argc Args vector's count
 * @param argv Args vector
 * @p argv and @p argc should NOT include program name
 * @return Pointer to handler function (nullptr if command doesn't exists)
   and name of the command passed through argv
*/
extern std::pair<handler_fn, std::string_view> parse(int argc, char **argv);

/**
 * @brief Print whole program's commands and aliases
 * @param program_name Program's name, argv[0]
 * @param indent Indentation level to be used at printing
 * @param f Stream to print (stdout by default)
 * It'll print commands like this:
 * <indentation><command_name> [ | <...aliases seperated by '|'> ]<usage>: <description>
*/
void print_commands(const char *program_name, int indent = 4, std::FILE *f = stdout);

/**
 * @brief Print a command or alias
 * @param program_name Program's name, argv[0]
 * @param indent Indentation level to be used at printing
 * @param f Stream to print (stdout by default)
 * It'll print commands like this:
 * <command_name>:
 * <indent>Usage: <program_name> <command_name> <usage>
 * <indent>Description: <description>
 * <indent>Aliases: <...aliases seperated by ','>
*/
extern void print_command(std::string_view cmd_name, const char *program_name, int indent = 4, std::FILE *f = stdout);

// Implementation
#if defined(SUBCMD_IMPL) || defined(SUBCMD_IMPLEMENTATION)

#define _SUBCMD_SV_ARG(sv) (int)(sv).size(), (sv).data()

static std::string_view resolve_alias(std::string_view alias)
{
  if (auto it = als.find(alias); it != als.end()) {
    return it->second.to;
  } else return alias;
}

template <typename... Args>
void log(Loc loc, std::string_view level, std::format_string<Args...> fmt, Args &&...args)
{
  std::cerr << loc.file_name() << ":" << loc.line() << ":" << loc.column()
    << " " << level << ": " << std::format(fmt, std::forward<Args>(args)...) << std::endl;
}

template <typename... Args>
void log_fatal(Loc loc, std::format_string<Args...> fmt, Args &&...args)
{ log(loc, "fatal", fmt, std::forward<Args>(args)...); }

template <typename... Args>
void log_note(Loc loc, std::format_string<Args...> fmt, Args &&...args)
{ log(loc, "note", fmt, std::forward<Args>(args)...); }

const char *shift(int &argc, char **&argv)
{ return argc == 0 ? nullptr : (argc--, *argv++); }

void add_command(Command &cmd, Loc loc)
{
#ifdef SUBCMD_DEBUG
  if (auto it = cmds.find(cmd.name); it != cmds.end()) {
    log_fatal(loc, "command '{}' already registered!", cmd.name);
    log_note(it->second.defined_at, "previous registration was here");
    std::abort();
  }
#endif

  cmd.defined_at = loc;
  cmds[cmd.name] = cmd;
}

void add_commands(std::initializer_list<Command> commands, Loc loc)
{
  for (Command c : commands) add_command(c, loc);
}

void add_alias(Alias al, Loc loc)
{
#ifdef SUBCMD_DEBUG
  if (auto it = als.find(al.alias); it != als.end()) {
    log_fatal(loc, "alias '{}' already registered", al.alias);
    log_note(it->second.defined_at, "previous registration was here");
    std::abort();
  }

  if (!cmds.contains(al.to)) {
    log_fatal(loc, "alias '{}' points to non-existing command", al.to);
    std::abort();
  }

  if (cmds.contains(al.alias)) {
    log_fatal(loc, "alias '{}' collides with a command", al.alias);
    std::abort();
  }
#endif

  al.defined_at = loc;
  als[al.alias] = std::move(al);
}

void add_aliases(std::initializer_list<Alias> aliases, std::source_location loc)
{
  for (Alias a : aliases) add_alias(a, loc);
}

std::pair<handler_fn, std::string_view> parse(int argc, char **argv)
{
  const char *name_cstr = shift(argc, argv);
  if (!name_cstr) return {nullptr, "<no command>"};
  std::string_view name = name_cstr;
  std::string_view target = resolve_alias(name);

  // Find command with resolved name
  // but return command name with its original value (unresolved)
  if (auto it = cmds.find(target); it != cmds.end()) {
    return {it->second.handler, name};
  }
  return {nullptr, name};
}

void print_command(std::string_view cmd_name, const char *program_name, int indent, std::FILE *f)
{
  Command cmd = cmds[resolve_alias(cmd_name)];

  std::fprintf(f, "%.*s:\n", _SUBCMD_SV_ARG(cmd.name));
  for (int i = 0; i < indent; i++) std::fputc(' ', f);

  std::fprintf(f, "Usage: %s %.*s %.*s\n", program_name,
          _SUBCMD_SV_ARG(cmd.name), _SUBCMD_SV_ARG(cmd.usage));

  for (int i = 0; i < indent; i++) std::fputc(' ', f);
  std::fprintf(f, "Description: %.*s\n", _SUBCMD_SV_ARG(cmd.desc));

  for (int i = 0; i < indent; i++) std::fputc(' ', f);
  std::fprintf(f, "Aliases: ");
  bool head = true;
  for (const auto &[_, al] : als) {
    if (al.to == cmd_name) {
      if (head) head = false;
      else std::fprintf(f, ", ");
      std::fprintf(f, "%.*s", _SUBCMD_SV_ARG(al.alias));
    }
  }
  std::fputc('\n', f);
}

void print_commands(const char *program_name, int indent, std::FILE *f)
{
  (void)program_name; (void)indent; (void)f;
  assert(false && "TODO: print_commands");
}

#endif // SUBCMD_IMPL

} // namespace subcmd

#endif // SUBCMD_HPP
