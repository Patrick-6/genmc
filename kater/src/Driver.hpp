#ifndef DRIVER_HPP
# define DRIVER_HPP
# include <memory>
# include <string>
# include <unordered_map>
# include "Parser.hpp"

# define YY_DECL yy::parser::symbol_type yylex (Driver& drv)
YY_DECL;

class Driver
{
public:

  std::unordered_map<std::string, std::unique_ptr<RegExp> > variables {};
  std::vector<std::unique_ptr<RegExp> > acyclicity_constraints {};
  std::vector<std::unique_ptr<RegExp> > emptiness_assumptions {};

  int result;
  bool debug;

  // Run the parser on file F.  Return 0 on success.
  int parse (const std::string& f);

  std::string file;

  void scan_begin();
  void scan_end();
  yy::location location;
};

#endif /* DRIVER_HPP */
