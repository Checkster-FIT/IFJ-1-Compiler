/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    debug.c
 * @brief   Various debug and logging functionality
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "debug.h"
#include "parser.h"
#include "scanner.h"
#include "stack.h"

void log_parsed(ParserContext* parser)
{
    if (parser->accepted.type == NONE)
        return;

    const char* prefix;
    const char* color = "\033[1;30m";

    // Accepted tokens (Recursive descent)
    if (parser->exit_code == 0 && parser->token_error == 0) {
        prefix = "OK ";
        color = "\033[1;32m";
        // Skipped tokens
        if (parser->debug_prefix_skipped) {
            prefix = "----";
        }
        // Accepted tokens (Terms inside expressions)
        else if (parser->debug_prefix_term) {
            prefix = "TRM";
            color = "\033[1;34m";
        }
        // Accepted tokens (Expressions)
        else if (parser->expressions_nested > 0) {
            prefix = "EXP";
            color = "\033[1;34m";
        }
        // Accepted tokens (With AST related data)
        else if (parser->debug_prefix_ast) {
            prefix = "OK+";
            color = "\033[1;32m";
        }
    }
    // Invalid token
    else if (parser->debug_prefix_failed) {
        prefix = "ERR";
        color = "\033[1;31m";
        parser->debug_prefix_failed = false;
    }
    // Unprocessed tokens
    else if (parser->debug_print_remaining >= 0) {
        prefix = "   ";
    }
    // Hidden tokens
    else {
        return;
    }

    if (parser->debug_prefix_term && parser->debug_indent_terms)
        for (int i = 1; i < parser->expressions_nested; i++)
            fprintf(stderr, " \033[1;30m---\033[0m");

    fprintf(stderr, " %s%s\033[0m  ", color, prefix);
    print_token(&parser->accepted);

    parser->debug_prefix_skipped = false;
}

void log_remaining(ParserContext* parser)
{
    while (true) {
        next(parser);

        parser->debug_print_remaining--;

        if (parser->next.type == END_OF_FILE) {
            next(parser);
            break;
        }
    }

    if (parser->debug_print_remaining >= 0)
        return;

    fprintf(stderr, "[***] %i more remaining\n", -(parser->debug_print_remaining));
}

// =======================================================

void log_precedence_stack_reverse(const StackEl* current)
{
    if (current == NULL || current->data == NULL)
        return;

    const PrecedenceElement* element = current->data;

    log_precedence_stack_reverse(current->next);

    static const char* groups[] = {
        [P_EXPRESSION] = "E",
        [P_L_BRACKET] = "(",
        [P_TERM] = "t",
        [P_R_BRACKET] = ")",
        [P_DELIMITER] = "$",
        [P_SPECIAL_START] = "<",
        [P_SPECIAL_END] = ">",
        [P_SPECIAL_EQUAL] = "=",
        [P_SPECIAL_INVALID] = "?",
    };

    const char* str;
    if (element->operation != OP_NONE)
        str = str_operation(element->operation);
    else
        str = map_str(groups, sizeof(groups) / sizeof(groups[0]),
            element->group, "?");

    fprintf(stderr, "%s", str);
}

void log_precedence_stack(const ParserContext* parser, const Stack* stack)
{
    if (!parser->debug_stack)
        return;

    if (parser->debug_indent_terms)
        for (int i = 1; i < parser->expressions_nested; i++)
            fprintf(stderr, "    ");

    fprintf(stderr, "      $");
    log_precedence_stack_reverse(stack->top);
    fprintf(stderr, "\n");
}

void print_line(ParserContext* parser)
{
    if (parser->exit_code != 0 || parser->token_error != 0)
        return;

    const char* divider = "-----------------------";
    fprintf(stderr, "%s [line %i] %s\n", divider, get_line(parser), divider);
}

// =======================================================

void log_ast_start(ParserContext* parser)
{
    if (!parser->debug)
        return;

    parser->debug_prefix_ast = true;
}

void log_ast_end(ParserContext* parser)
{
    if (parser->debug == false)
        return;

    parser->debug_prefix_ast = false;

    if (!parser->debug_ast_nodes)
        return;

    const ASTNode* node = parser->ast->program.recent_node;
    // static const char* format = "[\033[1;35m%s\033[0m] %-*s%s\n";
    static const char* format = "[\033[1;35m%s\033[0m] %-*s\n";

    const char* prefix = "AST";
    switch (node->type) {
        case AST_FUNCTION:
            fprintf(stderr, format, prefix, ALIGNMENT, "Function", node->function.identifier);
            break;
        case AST_EXTENSION:
            fprintf(stderr, format, prefix, ALIGNMENT, "Extension", "");
            break;
        case AST_VAR_DEF:
            fprintf(stderr, format, prefix, ALIGNMENT, "VarDef", node->vardef.identifier);
            break;
        case AST_PARAMETER:
            fprintf(stderr, format, prefix, ALIGNMENT, "Parameter", node->parameter.identifier);
            break;
        default:
            break;
    }
}

// =======================================================

void print_ast(ASTNode* root)
{
    Branch branch = {
        .parent = NULL,
        .symtable = root->program.global_symtable,
        .followed = false,
        .last = true
    };

    print_ast_recursively(root, &branch);
}

void print_ast_indent(const Branch* branch)
{
    if (!branch->parent)
        return;

    print_ast_indent(branch->parent);
    fprintf(stderr, "\033[1;30m%s\033[0m", branch->last ? "       " : "  │    ");
}

int print_ast_recursively(ASTNode* node, Branch* parent)
{
    if (!node)
        return 0;

    parent->last = !node->next && !parent->followed;

    const bool print_parent = false;
    if (print_parent && node->parent)
        fprintf(stderr, "%p ", node->parent);
    else if (print_parent)
        fprintf(stderr, "0x???????????? ");

    const bool print_line = true;
    if (print_line && node->line != 0)
        fprintf(stderr, "\033[1;30m%03d\033[0m", node->line);
    else if (print_line)
        fprintf(stderr, "\033[1;30m000\033[0m");

    if (parent->parent) {
        // Add one more line before functions
        if (node->type == AST_FUNCTION
            && (node->parent->program.extensions.last != NULL
                || node->parent->program.functions.first != node)) {
            print_ast_indent(parent->parent);
            fprintf(stderr, "\033[1;30m%s\033[0m", "  │    ");
            fputc('\n', stderr);

            if (print_parent)
                fprintf(stderr, "%p ", node->parent);
            if (print_line)
                fprintf(stderr, "\033[1;30m%03d\033[0m", node->line);
        }

        print_ast_indent(parent->parent);
        fprintf(stderr, "\033[1;30m%s\033[0m", parent->last ? "  └─────" : "  ├─────");
    }

    char* color = "\033[1;34m";
    char* color_secondary = "\033[1;32m";
    char* reset = "\033[0m";
    const SymtableEntry* entry = NULL;

    Branch
        this = {
            .parent = parent,
            .symtable = NULL,
            .followed = false,
            .last = true
        };

    switch (node->type) {
        case AST_NONE:
            fprintf(stderr, "[???] : Unknown node\n");
            break;

        case AST_PROGRAM:
            fprintf(stderr, " [%sAST%s] \n", color_secondary, reset);
            this.symtable = node->program.global_symtable;

            this.followed = node->program.functions.first != NULL;
            print_ast_recursively(node->program.extensions.first, &this);
            this.followed = false;
            print_ast_recursively(node->program.functions.first, &this);
            break;

        case AST_FUNCTION:
            entry = get_closest_symtable_entry(node, node->function.identifier);
            fprintf(stderr, "[%sFun%s] ", color_secondary, reset);
            if (entry)
                fprintf(stderr, "%s : %s", entry->identifier, str_type(entry->data_type));
            if (entry && entry->parameters)
                fprintf(stderr, " <%s>", entry->parameters->chars);
            if (entry && entry->main)
                fprintf(stderr, " (main)");
            fprintf(stderr, "\n");

            this.symtable = node->function.symtable;

            this.followed = node->function.statements.first != NULL;
            print_ast_recursively(node->function.parameter_groups.first, &this);
            this.followed = false;
            print_ast_recursively(node->function.statements.first, &this);
            break;

        case AST_EXTENSION:
            fprintf(stderr, "[%sExt%s] \n", color_secondary, reset);
            this.symtable = node->extension.symtable;

            this.followed = node->extension.functions.first != NULL;
            print_ast_recursively(node->function.parameter_groups.first, &this);
            this.followed = false;
            print_ast_recursively(node->extension.functions.first, &this);
            break;

        case AST_PARAMETER_GROUP:
            fprintf(stderr, "[%sParams%s] \n", color_secondary, reset);
            print_ast_recursively(node->parameter_group.parameters.first, &this);
            break;
        case AST_PARAMETER:
            entry = get_closest_symtable_entry(node, node->parameter.identifier);
            fprintf(stderr, "[%sPar%s] ", color_secondary, reset);
            if (entry)
                fprintf(stderr, "%s : %s\n", entry->identifier, str_type(entry->data_type));
            break;

        case AST_VAR_DEF:
            entry = get_closest_symtable_entry(node, node->vardef.identifier);
            if (entry && !entry->constant)
                fprintf(stderr, "[%sVar%s] ", color, reset);
            else if (entry)
                fprintf(stderr, "[%sVal%s] ", color, reset);
            else
                fprintf(stderr, "[%sV??%s] ", color, reset);
            if (entry)
                fprintf(stderr, "%s : %s\n", entry->identifier, str_type(entry->data_type));
            print_ast_recursively(node->vardef.expression, &this);
            break;

            break;
        case AST_EXPRESSION:
            if (node->expression.operation != OP_NONE)
                fprintf(stderr, "[%s%s%s]", color, str_operation(node->expression.operation), reset);
            else
                fprintf(stderr, "[%s?%s]", color, reset);
            fprintf(stderr, " : %s\n", str_type(node->expression.result_type));
            this.followed = node->expression.right != NULL;
            print_ast_recursively(node->expression.left, &this);
            this.followed = false;
            print_ast_recursively(node->expression.right, &this);
            break;

        case AST_LITERAL:
            switch (node->literal.type) {
                case AST_INTEGER:
                    fprintf(stderr, "[%sInt%s]", color, reset);
                    fprintf(stderr, " %i\n", node->literal.val_integer);
                    break;
                case AST_DOUBLE:
                    fprintf(stderr, "[%sDouble%s]", color, reset);
                    fprintf(stderr, " %lf\n", node->literal.val_double);
                    break;
                case AST_BOOLEAN:
                    fprintf(stderr, "[%sBool%s]", color, reset);
                    fprintf(stderr, " %s\n", node->literal.val_bool ? "true" : "false");
                    break;
                case AST_STRING:
                    fprintf(stderr, "[%sString%s] \"", color, reset);
                    fprintf_escaped(stderr, node->literal.val_string);
                    fprintf(stderr, "\"\n");
                    break;
                case AST_UNIT:
                    fprintf(stderr, "[%sUnit%s]", color, reset);
                    fprintf(stderr, " ()\n");
                    break;
                case AST_UNKNOWN_LITERAL:
                    fprintf(stderr, "[%s...%s]\n", color, reset);
                    break;
            }
            break;

        case AST_VAR_ASSIGN:
            entry = get_closest_symtable_entry(node, node->vardef.identifier);
            fprintf(stderr, "[%s=%s] ", color, reset);
            if (entry)
                fprintf(stderr, "%s : %s\n", entry->identifier, str_type(entry->data_type));
            print_ast_recursively(node->assignment.expression, &this);
            break;

        case AST_WHILE_DO:
            fprintf(stderr, "[%sWhile%s] \n", color_secondary, reset);
            this.symtable = node->while_do.symtable;

            this.followed = node->while_do.statements.first != NULL;
            print_ast_recursively(node->while_do.condition, &this);
            this.followed = false;
            print_ast_recursively(node->function.statements.first, &this);
            break;

        case AST_RETURN:
            fprintf(stderr, "[%sRet%s] \n", color, reset);
            print_ast_recursively(node->return_st.expression, &this);
            break;

        case AST_INTERPOLATED_STRING:
            fprintf(stderr, "[%sIString%s]", color, reset);
            fprintf(stderr, " : %s\n", str_type('s'));
            print_ast_recursively(node->interpolated_string.expressions.first, &this);
            break;

        case AST_IF:
            fprintf(stderr, "[%sIf%s] \n", color_secondary, reset);
            this.followed = node->if_statement.then_node != NULL;
            print_ast_recursively(node->if_statement.condition, &this);
            this.followed = node->if_statement.else_node != NULL;
            print_ast_recursively(node->if_statement.then_node, &this);
            this.followed = false;
            print_ast_recursively(node->if_statement.else_node, &this);
            break;
        case AST_THEN:
            fprintf(stderr, "[%sThen%s] \n", color_secondary, reset);
            print_ast_recursively(node->then_statement.statements.first, &this);
            break;
        case AST_ELSE:
            fprintf(stderr, "[%sElse%s] \n", color_secondary, reset);
            print_ast_recursively(node->else_statement.statements.first, &this);
            break;
        case AST_CONVERSION:
            fprintf(stderr, "[%sConv%s] ", color, reset);
            fprintf(stderr, "<%c>\n", node->conversion.result_type);
            print_ast_recursively(node->conversion.expression, &this);
            break;
        case AST_ARGUMENT_GROUP:
            fprintf(stderr, "[%sArgs%s] \n", color, reset);
            print_ast_recursively(node->argument_group.arguments.first, &this);
            break;

        case AST_TERM:
        case AST_BUILTIN_FUNCALL:
        case AST_FUNCALL:
        case AST_VAR_USE:
            entry = get_closest_symtable_entry(node, node->term.identifier);

            if (node->type == AST_TERM)
                fprintf(stderr, "[%sTerm%s]", color, reset);
            else if (node->type == AST_FUNCALL || node->type == AST_BUILTIN_FUNCALL)
                fprintf(stderr, "[%sCall%s]", color, reset);
            else if (node->type == AST_VAR_USE)
                fprintf(stderr, "[%sUse%s]", color, reset);

            fprintf(stderr, " %s", node->term.identifier);
            if (entry)
                fprintf(stderr, " : %s", str_type(entry->data_type));
            if (entry && entry->parameters)
                fprintf(stderr, " <%s>", entry->parameters->chars);
            if (node->type == AST_BUILTIN_FUNCALL)
                fprintf(stderr, " (built-in)");
            if (entry && entry->extension)
                fprintf(stderr, " (extension)");
            fprintf(stderr, "\n");

            print_ast_recursively(node->term.argument_groups.first, &this);
            break;
    }
    print_ast_recursively(node->next, parent);
    return 0;
}

const char* str_operation(const Operator op)
{
    static const char* map[] = {
        [OP_NOT] = "!",
        [OP_UNARY_PLUS] = "+",
        [OP_UNARY_MINUS] = "-",
        [OP_MUL] = "*",
        [OP_DIV] = "/",
        [OP_ADD] = "+",
        [OP_SUB] = "-",
        [OP_LESS] = "<",
        [OP_LESS_EQUAL] = "<=",
        [OP_GREATER] = ">",
        [OP_GREATER_EQUAL] = ">=",
        [OP_EQUAL] = "==",
        [OP_NOT_EQUAL] = "!=",
        [OP_AND] = "&&",
        [OP_OR] = "||",
        [OP_DOT] = "."
    };

    return map_str(map, sizeof(map) / sizeof(map[0]), op, "E");
}

// =======================================================

const char* str_type(const char type_char)
{
    switch (type_char) {
        case 's':
            return "String";
        case 'i':
            return "Int";
        case 'd':
            return "Double";
        case 'u':
            return "Unit";
        case 'b':
            return "Boolean";
        default:
            return "Unknown";
    }
}

const char* str_token(const Token* token, const TokenStringType mode)
{
    static const char* name_map[] = {
        [NONE] = "NONE",
        [ERROR] = "ERROR",
        [INTEGER] = "INTEGER",
        [DOUBLE] = "DOUBLE",
        [OPERATOR] = "OPERATOR",
        [IDENTIFIER] = "IDENTIFIER",
        [MAIN_IDENTIFIER] = "MAIN_IDENTIFIER",
        [KEYWORD] = "KEYWORD",
        [BOOLEAN] = "BOOLEAN",
        [STRING_INTERPOLATION] = "STRING_INTERPOLATION",
        [STRING_INTERPOLATION_END] = "STRING_INTERPOLATION_END",
        [MULTILINE_STRING_INTERPOLATION] = "MULTILINE_STRING_INTERPOLATION",
        [MULTILINE_STRING_INTERPOLATION_END] = "MULTILINE_STRING_INTERPOLATION_END",
        [STRING] = "STRING",
        [MULTILINE_STRING] = "MULTILINE_STRING",
        [DOT] = "DOT",
        [DECLARATION_SIGN] = "DECLARATION_SIGN",
        [COMMA] = "COMMA",
        [SEMICOLON] = "SEMICOLON",
        [INDENT] = "INDENT",
        [DEDENT] = "DEDENT",
        [LINE_FEED] = "LINE_FEED",
        [WHITESPACE] = "WHITESPACE",
        [ASSIGNMENT] = "ASSIGNMENT",
        [COMPARE] = "COMPARE",
        [LEFT_BRACKET] = "LEFT_BRACKET",
        [RIGHT_BRACKET] = "RIGHT_BRACKET",
        [END_OF_FILE] = "END_OF_FILE"
    };

    static char num_to_str_buffer[BUFFER_SIZE];

    switch (token->type) {
        case INTEGER:
            snprintf(num_to_str_buffer, BUFFER_SIZE, "%li", token->attribute_int);
            break;
        case DOUBLE:
            snprintf(num_to_str_buffer, BUFFER_SIZE, "%lf", token->attribute_double);
            break;
        case ERROR:
            snprintf(num_to_str_buffer, BUFFER_SIZE, "%i", token->attribute_error);
            break;
        default:;
    }

    const char* attribute_map[] = {
        [DOT] = ".",
        [DECLARATION_SIGN] = ":",
        [SEMICOLON] = ";",
        [COMMA] = ",",
        [ASSIGNMENT] = "=",
        [LEFT_BRACKET] = "(",
        [RIGHT_BRACKET] = ")",
        [KEYWORD] = str_keyword(token->attribute_keyword),
        [OPERATOR] = str_token_operator(token->attribute_operation_type),
        [COMPARE] = str_compare(token->attribute_compare_type),
        [BOOLEAN] = (token->attribute_bool ? "true" : "false"),
        [IDENTIFIER] = token->attribute_string,
        [MAIN_IDENTIFIER] = token->attribute_string,
        [STRING] = token->attribute_string,
        [STRING_INTERPOLATION] = token->attribute_string,
        [STRING_INTERPOLATION_END] = token->attribute_string,
        [MULTILINE_STRING] = token->attribute_string,
        [MULTILINE_STRING_INTERPOLATION] = token->attribute_string,
        [MULTILINE_STRING_INTERPOLATION_END] = token->attribute_string,
        [INTEGER] = num_to_str_buffer,
        [DOUBLE] = num_to_str_buffer,
        [ERROR] = num_to_str_buffer
    };

    int size = 0;
    switch (mode) {
        case GET_NAME:
            size = sizeof(name_map) / sizeof(name_map[0]);
            return map_str(name_map, size, token->type, "[Unknown token]");
        case GET_ATTRIBUTE:
            size = sizeof(attribute_map) / sizeof(attribute_map[0]);
            return map_str(attribute_map, size, token->type, "");
        default:
            return "unknown";
    }
}

// =======================================================

void print_token(const Token* token)
{
    const char* name = str_token(token, GET_NAME);
    const char* attribute = str_token(token, GET_ATTRIBUTE);

    fprintf(stderr, "%-*s", ALIGNMENT, name);

    if (is_string_token(token))
        fprintf(stderr, "\"");

    fprintf_escaped(stderr, attribute);

    if (is_string_token(token))
        fprintf(stderr, "\"");

    fprintf(stderr, "\n");
}

// =======================================================

bool is_string_token(const Token* token)
{
    switch (token->type) {
        case STRING:
        case STRING_INTERPOLATION:
        case STRING_INTERPOLATION_END:
        case MULTILINE_STRING:
        case MULTILINE_STRING_INTERPOLATION:
        case MULTILINE_STRING_INTERPOLATION_END:
            return true;

        default:
            return false;
    }
}

// =======================================================

const char* str_keyword(const int idx)
{
    static const char* map[] = {
        [DEF_KW] = "def",
        [DO_KW] = "do",
        [DOUBLE_KW] = "Double",
        [ELSE_KW] = "else",
        [IF_KW] = "if",
        [IMPORT_KW] = "import",
        [INT_KW] = "Int",
        [RETURN_KW] = "return",
        [STRING_KW] = "String",
        [THEN_KW] = "then",
        [UNIT_KW] = "Unit",
        [VAL_KW] = "val",
        [VAR_KW] = "var",
        [WHILE_KW] = "while",
        [BOOLEAN_KW] = "Boolean",
        [CASE_KW] = "case",
        [CLASS_KW] = "class",
        [EXTENSION_KW] = "extension",
        [FOR_KW] = "for",
        [LIST_KW] = "List",
        [MATCH_KW] = "match",
        [TYPE_KW] = "type"
    };

    return map_str(map, sizeof(map) / sizeof(map[0]), idx, "[Unknown keyword]");
}

const char* str_token_operator(const int idx)
{
    static const char* map[] = {
        [PLUS] = "+",
        [MINUS] = "-",
        [MULTIPLY] = "*",
        [AND] = "&&",
        [OR] = "||",
        [NOT] = "!",
        [DIVIDE] = "/"
    };

    return map_str(map, sizeof(map) / sizeof(map[0]), idx, "[Unknown operator]");
}

const char* str_compare(const int idx)
{
    static const char* map[] = {
        [EQUAL] = "==",
        [NOT_EQUAL] = "!=",
        [LESS] = "<",
        [GREATER] = ">",
        [LESS_EQUAL] = "<=",
        [GREATER_EQUAL] = ">="
    };

    return map_str(map, sizeof(map) / sizeof(map[0]), idx, "[Unknown comparator]");
}

// =======================================================

void fprintf_escaped(FILE* stream, const char* string)
{
    if (string == NULL)
        return;

    for (size_t idx = 0; string[idx] != '\0'; idx++) {
        if (string[idx] == '\n')
            fprintf(stream, "\\n");
        else
            fputc(string[idx], stream);
    }
}

const char* map_str(const char** map, const int size, const int idx, const char* fallback)
{
    if (0 <= idx && idx < size && map[idx] != NULL)
        return map[idx];

    return fallback;
}