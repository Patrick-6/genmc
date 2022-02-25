%skeleton "lalr1.cc" // -*- C++ -*-
%require "3.8.1"
%header

%define api.token.raw
%define api.token.constructor
%define api.token.prefix {TOK_}
%define api.value.type variant
%define parse.assert
%define parse.trace
%define parse.error detailed
%define parse.lac full
%locations

%code requires {
  #include <string>
  #include "RegExp.hpp"
  class Driver;
}

%param { Driver& drv }

%code {
  #include <iostream>
  #include <unordered_map>
  #include <vector>
  #include "RegExp.hpp"
  #include "Driver.hpp"
}


%token
  LET	   "let"
  ACYCLIC  "acyclic"
  PROPERTY "property"
  ZERO     "0"
  EQ       "="
  SEMI     ";"
  ALT      "|"
  PLUS     "+"
  STAR     "*"
  QMARK    "?"
  LPAREN   "("
  RPAREN   ")"
  LBRACK   "["
  RBRACK   "]"
;

%token <std::string> ID;
%nterm <RegExp *> rel;

%left  "|"
%left  ";"
%precedence "+" "*" "?"

%printer { yyo << *($$); } <RegExp *>;
%printer { yyo << $$; } <*>;

%start main
%%

main:	  %empty		{ }
	| main decl		{ }
        ;

decl:	  "let" ID "=" rel		{ drv.variables[$2] = std::unique_ptr<RegExp>($4); }
	| "property" rel "=" "0" 	{ drv.emptiness_assumptions.push_back(std::unique_ptr<RegExp>($2)); }
	| "acyclic" rel 		{ drv.acyclicity_constraints.push_back(std::unique_ptr<RegExp>($2)); }
	;

rel:	  "(" rel ")"		{ $$ = $2; }
	| "[" rel "]"		{ $$ = make_BracketRE($2); }
	| rel "|" rel		{ $$ = new AltRE($1, $3); }
	| rel ";" rel		{ $$ = new SeqRE($1, $3); }
	| rel "*"		{ $$ = new StarRE($1, 0); }
	| rel "+"		{ $$ = new PlusRE($1, 0); }
	| rel "?"		{ $$ = new QMarkRE($1, 0); }
   	| ID			{ auto it = drv.variables.find($1);
				  $$ = (it == drv.variables.end()) ? new CharRE($1) : it->second->clone(); } 
	; 

%%

void yy::parser::error (const location_type& l, const std::string& m)
{
	std::cerr << l << ": " << m << '\n';
}
