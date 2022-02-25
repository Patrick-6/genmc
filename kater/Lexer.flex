%{
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <string>
#include "Driver.hpp"
#include "Parser.hpp"

#define YY_USER_ACTION  loc.columns (yyleng);
%}

%option nodefault
%option noyywrap nounput noinput batch

DIGIT	[0-9]+
ALPHA	[_a-zA-Z]
EALPHA	[-'._0-9a-zA-Z]
BLANK	[ \t\r]

%%
%{
	yy::location& loc = drv.location;
	loc.step ();
%}

let			return yy::parser::make_LET (loc);
acyclic			return yy::parser::make_ACYCLIC (loc);
property		return yy::parser::make_PROPERTY (loc);

{ALPHA}{EALPHA}*	return yy::parser::make_ID     (yytext, loc);

"+"			return yy::parser::make_PLUS   (loc);
"*"			return yy::parser::make_STAR   (loc);
"?"			return yy::parser::make_QMARK  (loc);
";"			return yy::parser::make_SEMI   (loc);
"|"			return yy::parser::make_ALT    (loc);
"("			return yy::parser::make_LPAREN (loc);
")"			return yy::parser::make_RPAREN (loc);
"["			return yy::parser::make_LBRACK (loc);
"]"			return yy::parser::make_RBRACK (loc);
"="			return yy::parser::make_EQ     (loc);
"0"			return yy::parser::make_ZERO   (loc);
{BLANK}+		loc.step ();
\n+			loc.lines (yyleng); loc.step ();
"//"[^\n]*\n		loc.lines (yyleng); loc.step ();

.			{ throw yy::parser::syntax_error
			    (loc, "invalid character: " + std::string(yytext)); }

<<EOF>>			return yy::parser::make_YYEOF (loc);

%%

//0			return yy::parser::make_EMPTY       (loc);
//-1			return yy::parser::make_MINUSONE    (loc);

void Driver::scan_begin ()
{
	if (file.empty () || file == "-")
		yyin = stdin;
	else if (!(yyin = fopen (file.c_str (), "r"))) {
		std::cerr << "cannot open " << file << ": " << strerror (errno) << '\n';
		exit (EXIT_FAILURE);
	}
}

void Driver::scan_end ()
{
	fclose (yyin);
}
