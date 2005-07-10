#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "parser.h"

#include "eacompiler.h"

#include "basesafeptr.h"




void usage(const char*p)
{
	printf("usage: %s <.cgt file> <input file>\n", (p)?p:"<binary>");
}


void vartest();


// Accepts 2 arguments <.cgt file> <input file>
int main(int argc, char *argv[])
{
	vartest();


	CScriptEnvironment env;
	CScriptCompiler compiler(env);


	ulong tick = GetTickCount();

	bool run = true;
	FILE*		fin = 0;
	CParser* parser = 0;
	CParseConfig* parser_config = 0;

	if (argc < 3){
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	// Open .cgt file
	parser_config = new CParseConfig(argv[1]);
	if (!parser_config){
		printf("Could not open cgt file %s\n", argv[1]);
		return EXIT_FAILURE;
	}
	parser = new CParser(parser_config);
	if (!parser){
		printf("Error creating parser\n");
		return EXIT_FAILURE;
	}

	// Open input file
	if( !parser->input.open(argv[2]) ) {
		printf("Could not open input file %s\n", argv[1]);
		return EXIT_FAILURE;
	}


	
	while(run)
	{
		short p = parser->parse();
		if (p < 0)
		{	// an error
			printf("Parse Error: line %i, col %i\n", parser->input.line, parser->input.column);

			parser->print_expects();

			run = false;
		}
		else if(0 == p)
		{	// finished
			run = false;
		}
		
		
		if( parser->rt[0].symbol.idx==PT_DECL && parser->rt[0].rtchildren.size() )
		{
			CStackElement *child = &(parser->rt[(parser->rt[0].rtchildren[0])]);
			if( child &&
				( child->symbol.idx == PT_SCRIPTDECL ||
				  child->symbol.idx == PT_BLOCK ||
				  child->symbol.idx == PT_FUNCDECL ||
				  child->symbol.idx == PT_FUNCPROTO ) )
			{
				
				printf("(%i)----------------------------------------\n", parser->rt.size());
				parser->print_rt_tree(0,0);


				//////////////////////////////////////////////////////////
				// tree transformation
				parsenode pnode(*parser);
				pnode.print_tree();


				//////////////////////////////////////////////////////////
				// compiling
				if( !compiler.CompileTree(pnode) )
					run = false;

				
				//////////////////////////////////////////////////////////
				// reinitialize parser
				parser->reinit();
				printf("............................................(%i)\n", global::getcount());
			}
		}
	}

	// Print parse tree
//	parser->print_rt_tree(0, 0);
	printf("\nready\n");
	if (parser)  delete parser;
	if (parser_config) delete parser_config;

	printf("time: %i", GetTickCount()-tick);

	return EXIT_SUCCESS;
}

