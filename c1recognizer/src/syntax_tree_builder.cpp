
#include "syntax_tree_builder.h"
#include <memory>

using namespace c1_recognizer;
using namespace c1_recognizer::syntax_tree;

syntax_tree_builder::syntax_tree_builder(error_reporter &_err) : err(_err) {}

antlrcpp::Any syntax_tree_builder::visitCompilationUnit(C1Parser::CompilationUnitContext *ctx)
{
    auto result = new assembly;
    result->line = ctx->getStart()->getLine();
    result->pos = ctx->getStart()->getCharPositionInLine();
    auto childrens = ctx->children;
    for(int i =0;i<childrens.size();i++){
        auto temp=childrens[i];
        if( auto temp2 = dynamic_cast<C1Parser::DeclContext *>(temp)){
            auto temp3 = visit(temp2).as<std::vector<var_def_stmt_syntax *>>();
            for(int j = 0;j<temp3.size();j++){
                result->global_defs.emplace_back(temp3[j]);
            }
        }
        else if(auto temp2 = dynamic_cast<C1Parser::FuncdefContext *>(temp)){
            auto temp3 = visit(temp2).as<func_def_syntax *>();
            result->global_defs.emplace_back(temp3);
        }
    }
    return result;
}

antlrcpp::Any syntax_tree_builder::visitDecl(C1Parser::DeclContext *ctx)
{
    if(auto dec = ctx->vardecl()){
        return visit(dec);
    }
    if(auto dec = ctx->constdecl()){
        return visit(dec);
    }
}

antlrcpp::Any syntax_tree_builder::visitConstdecl(C1Parser::ConstdeclContext *ctx)
{
    std::vector<var_def_stmt_syntax *> result;
    int i = 0;
    auto temp0 = ctx->constdef();
    for(auto &temp :temp0 ){
        auto temp2 = visit(temp).as<var_def_stmt_syntax *>();
        if(ctx->Int()){
        temp2->is_int = true;
        }
        if(ctx->Float()){
        temp2->is_int = false;
        }
        result.emplace_back(temp2);
        i++;
    }
    return result;
}

antlrcpp::Any syntax_tree_builder::visitConstdef(C1Parser::ConstdefContext *ctx)
{
    auto result = new var_def_stmt_syntax;
    auto explist = ctx->exp();
    result->line = ctx->getStart()->getLine();
    result->pos = ctx->getStart()->getCharPositionInLine();
    result->name = ctx->Identifier()->getSymbol()->getText();
    result->is_constant = true;
    if(ctx->LeftBracket()){
        result->array_length.reset(visit(explist[0]).as<expr_syntax *>());
        int i = 1;
        while(i<explist.size()){
            auto temp = explist[i];
            auto temp2 = visit(temp).as<expr_syntax *>();
            result->initializers.emplace_back(temp2);
            i++;
        }
    }
    else{
        result->array_length.reset();// set to null ptr
        auto temp2 = visit(explist[0]).as<expr_syntax *>();
        result->initializers.emplace_back(temp2);
    }
    return static_cast<var_def_stmt_syntax *>(result);
}

antlrcpp::Any syntax_tree_builder::visitVardecl(C1Parser::VardeclContext *ctx)
{
    std::vector<var_def_stmt_syntax *> result;
    int i = 0;
    auto vardeflist = ctx->vardef();
    for(auto &temp :vardeflist){
        auto temp2 = visit(temp).as<var_def_stmt_syntax *>();
        if(ctx->Int()){
        temp2->is_int = true;
        }
        if(ctx->Float()){
        temp2->is_int = false;
        }
        result.emplace_back(temp2);
        i++;
    }
    return result;
}

antlrcpp::Any syntax_tree_builder::visitVardef(C1Parser::VardefContext *ctx)
{
    auto result = new var_def_stmt_syntax;
    auto explist = ctx->exp();
    result->is_constant = false;
    result->line = ctx->getStart()->getLine();
    result->pos = ctx->getStart()->getCharPositionInLine();
    result->name = ctx->Identifier()->getSymbol()->getText();
    if(ctx->LeftBracket() && ctx->Assign()){
        //case ID[exp] = {...}
        result->array_length.reset(visit(explist[0]).as<expr_syntax *>());
        int i = 1;
        while(i<explist.size()){
            auto temp = explist[i];
            auto temp2 = visit(temp).as<expr_syntax *>();
            result->initializers.emplace_back(temp2);
            i++;
        }
    }
    else if(ctx->Assign()){
        //case ID = ...
        result->array_length.reset();
        auto temp2 = visit(explist[0]).as<expr_syntax *>();
        result->initializers.emplace_back(temp2);
    }
    else if(ctx->LeftBracket()){
        //case ID[exp]
        result->array_length.reset(visit(explist[0]).as<expr_syntax *>());
    }
    else{
        // case ID
        result->array_length.reset();
    }
    return static_cast<var_def_stmt_syntax *>(result);
}

antlrcpp::Any syntax_tree_builder::visitFuncdef(C1Parser::FuncdefContext *ctx)
{
    auto result = new func_def_syntax;
    result->line = ctx->getStart()->getLine();
    result->pos = ctx->getStart()->getCharPositionInLine();
    result->name = ctx->Identifier()->getSymbol()->getText();
    result->body.reset(visit(ctx->block()).as<block_syntax *>());
    return static_cast<func_def_syntax *>(result);
}

antlrcpp::Any syntax_tree_builder::visitBlock(C1Parser::BlockContext *ctx)
{
    auto result = new block_syntax;
    result->line = ctx->getStart()->getLine();
    result->pos = ctx->getStart()->getCharPositionInLine();
    auto children = ctx->children;
   for (int i=0;i< children.size();i++) {
        auto temp0 = children[i];
        if (auto child = dynamic_cast<C1Parser::DeclContext *>(temp0)){
            auto temp = visit(child).as<std::vector<var_def_stmt_syntax *> >();
            for(int i = 0; i < temp.size(); i++){
                result->body.emplace_back(temp[i]);

            }
        }
        else if(auto child = dynamic_cast<C1Parser::StmtContext *>(temp0)){
            result->body.emplace_back(visit(child).as<stmt_syntax *>());
        }
    }

    return static_cast<block_syntax  *>(result);
}

antlrcpp::Any syntax_tree_builder::visitStmt(C1Parser::StmtContext *ctx)
{
    if(auto temp = ctx->lval()){
        //assignment
        auto result = new assign_stmt_syntax;
        result->line = ctx->getStart()->getLine();
        result->pos = ctx->getStart()->getCharPositionInLine();
        result->target.reset(visit(temp).as<lval_syntax *>());
        result->value.reset(visit(ctx->exp()).as<expr_syntax *>());
        return static_cast<stmt_syntax *>(result);
    }
    else if(auto temp = ctx-> block()){
        //block
        return static_cast<stmt_syntax *>(visit(temp).as<block_syntax *>());
    }
    else if(ctx->If()){
        // If
        auto result = new if_stmt_syntax;
        result->line = ctx->getStart()->getLine();
        result->pos = ctx->getStart()->getCharPositionInLine();
        result->pred.reset(visit(ctx->cond()).as<cond_syntax *>());
        auto statem = ctx->stmt();
        result->then_body.reset(visit(statem[0]).as<stmt_syntax *>());
        if(ctx->Else()){
            result->else_body.reset(visit(statem[1]).as<stmt_syntax *>());
        }
        else{
            result->else_body.reset();
        }
        return static_cast<stmt_syntax *>(result);
    }
    else if(ctx->While()){
        // While
        auto result = new while_stmt_syntax;
        result->line = ctx->getStart()->getLine();
        result->pos = ctx->getStart()->getCharPositionInLine();
        auto statem = ctx->stmt();
        result->pred.reset(visit(ctx->cond()).as<cond_syntax *>());
        result->body.reset(visit(statem[0]).as<stmt_syntax *>());
        return static_cast<stmt_syntax *>(result);
    }
    else if(auto temp = ctx->Identifier()){
        // func call
        auto result = new func_call_stmt_syntax;
        result->line = ctx->getStart()->getLine();
        result->pos = ctx->getStart()->getCharPositionInLine();
        result->name=temp->getSymbol()->getText();
        return static_cast<stmt_syntax *>(result);
    }
    else {
        //empty
        auto result = new empty_stmt_syntax;
        return static_cast<stmt_syntax *>(result);
    }
}

antlrcpp::Any syntax_tree_builder::visitLval(C1Parser::LvalContext *ctx)
{
    auto result = new lval_syntax;
    result->name = ctx->Identifier()->getSymbol()->getText();
    result->line = ctx->getStart()->getLine();
    result->pos = ctx->getStart()->getCharPositionInLine();
    if(auto temp = ctx->exp()){
        result->array_index.reset(visit(temp).as<expr_syntax *>());
    }
    else{
        result->array_index.reset();
    }
    return static_cast<lval_syntax *>(result);
}

antlrcpp::Any syntax_tree_builder::visitCond(C1Parser::CondContext *ctx)
{
    auto expressions = ctx->exp();

    auto result = new cond_syntax;
    //set line&pos
    result->line = ctx->getStart()->getLine();
    result->pos = ctx->getStart()->getCharPositionInLine();
    //same as exp
    if(ctx->Equal())
        result->op = relop::equal;
    if(ctx->NonEqual())
        result->op = relop::non_equal;
    if(ctx->Less())
        result->op = relop::less;
    if(ctx->LessEqual())
        result->op = relop::less_equal;
    if(ctx->Greater())
        result->op = relop::greater;
    if(ctx->GreaterEqual())
        result->op = relop::greater_equal;
        //add left exp and right exp to node
    result->lhs.reset(visit(expressions[0]).as<expr_syntax *>());
    result->rhs.reset(visit(expressions[1]).as<expr_syntax *>());
    return static_cast<cond_syntax *>(result);
}

// Returns antlrcpp::Any, which is constructable from any type.
// However, you should be sure you use the same type for packing and depacking the `Any` object.
// Or a std::bad_cast exception will rise.
// This function always returns an `Any` object containing a `expr_syntax *`.
antlrcpp::Any syntax_tree_builder::visitExp(C1Parser::ExpContext *ctx)
{
    // Get all sub-contexts of type `exp`.
    auto expressions = ctx->exp();
    // Two sub-expressions presented: this indicates it's a expression of binary operator, aka `binop`.
    if (expressions.size() == 2)
    {
        auto result = new binop_expr_syntax;
        // Set line and pos.
        result->line = ctx->getStart()->getLine();
        result->pos = ctx->getStart()->getCharPositionInLine();
        // visit(some context) is equivalent to calling corresponding visit method; dispatching is done automatically
        // by ANTLR4 runtime. For this case, it's equivalent to visitExp(expressions[0]).
        // Use reset to set a new pointer to a std::shared_ptr object. DO NOT use assignment; it won't work.
        // Use `.as<Type>()' to get value from antlrcpp::Any object; notice that this Type must match the type used in
        // constructing the Any object, which is constructed from (usually pointer to some derived class of
        // syntax_node, in this case) returning value of the visit call.
        result->lhs.reset(visit(expressions[0]).as<expr_syntax *>());
        // Check if each token exists.
        // Returnd value of the calling will be nullptr (aka NULL in C) if it isn't there; otherwise non-null pointer.
        if (ctx->Plus())
            result->op = binop::plus;
        if (ctx->Minus())
            result->op = binop::minus;
        if (ctx->Multiply())
            result->op = binop::multiply;
        if (ctx->Divide())
            result->op = binop::divide;
        if (ctx->Modulo())
            result->op = binop::modulo;
        result->rhs.reset(visit(expressions[1]).as<expr_syntax *>());
        return static_cast<expr_syntax *>(result);
    }
    // Otherwise, if `+` or `-` presented, it'll be a `unaryop_expr_syntax`.
    if (ctx->Plus() || ctx->Minus())
    {
        auto result = new unaryop_expr_syntax;
        result->line = ctx->getStart()->getLine();
        result->pos = ctx->getStart()->getCharPositionInLine();
        if (ctx->Plus())
            result->op = unaryop::plus;
        if (ctx->Minus())
            result->op = unaryop::minus;
        result->rhs.reset(visit(expressions[0]).as<expr_syntax *>());
        return static_cast<expr_syntax *>(result);
    }
    // In the case that `(` exists as a child, this is an expression like `'(' expressions[0] ')'`.
    if (ctx->LeftParen())
        return visit(expressions[0]); // Any already holds expr_syntax* here, no need for dispatch and re-patch with casting.
    // If `number` exists as a child, we can say it's a literal integer expression.
    if (auto number = ctx->number())
        return visit(number);
    //If lcal exist as a child ...
    if(auto temp = ctx->lval()){
        auto temp2 = visit(temp).as<lval_syntax *>();
        return static_cast<expr_syntax *>(temp2);
    }
}

antlrcpp::Any syntax_tree_builder::visitNumber(C1Parser::NumberContext *ctx)
{
    auto result = new literal_syntax;
    if (auto intConst = ctx->IntConst())
    {
        result->is_int = true;
        result->line = intConst->getSymbol()->getLine();
        result->pos = intConst->getSymbol()->getCharPositionInLine();
        auto text = intConst->getSymbol()->getText();
        if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) // Hexadecimal
            result->intConst = std::stoi(text, nullptr, 16); // std::stoi will eat '0x'
        /* you need to add other situations here */
        else if(text[0]=='0'){
            result->intConst = std::stoi(text, nullptr, 8);
        }
        //octal case
        else{
            result->intConst = std::stoi(text, nullptr, 10); // std::stoi will eat '0x'
            }
        return static_cast<expr_syntax *>(result);
    }
    // else FloatConst
    else
    {
        result->is_int = false;
        result->line = ctx->FloatConst()->getSymbol()->getLine();
        result->pos = ctx->FloatConst()->getSymbol()->getCharPositionInLine();
        auto text = ctx->FloatConst()->getSymbol()->getText();
        result->floatConst = std::stod(text, nullptr);
        return static_cast<expr_syntax *>(result);
    }
}

ptr<syntax_tree_node> syntax_tree_builder::operator()(antlr4::tree::ParseTree *ctx)
{
    auto result = visit(ctx);
    if (result.is<syntax_tree_node *>())
        return ptr<syntax_tree_node>(result.as<syntax_tree_node *>());
    if (result.is<assembly *>())
        return ptr<syntax_tree_node>(result.as<assembly *>());
    if (result.is<global_def_syntax *>())
        return ptr<syntax_tree_node>(result.as<global_def_syntax *>());
    if (result.is<func_def_syntax *>())
        return ptr<syntax_tree_node>(result.as<func_def_syntax *>());
    if (result.is<cond_syntax *>())
        return ptr<syntax_tree_node>(result.as<cond_syntax *>());
    if (result.is<expr_syntax *>())
        return ptr<syntax_tree_node>(result.as<expr_syntax *>());
    if (result.is<binop_expr_syntax *>())
        return ptr<syntax_tree_node>(result.as<binop_expr_syntax *>());
    if (result.is<unaryop_expr_syntax *>())
        return ptr<syntax_tree_node>(result.as<unaryop_expr_syntax *>());
    if (result.is<lval_syntax *>())
        return ptr<syntax_tree_node>(result.as<lval_syntax *>());
    if (result.is<literal_syntax *>())
        return ptr<syntax_tree_node>(result.as<literal_syntax *>());
    if (result.is<stmt_syntax *>())
        return ptr<syntax_tree_node>(result.as<stmt_syntax *>());
    if (result.is<var_def_stmt_syntax *>())
        return ptr<syntax_tree_node>(result.as<var_def_stmt_syntax *>());
    if (result.is<assign_stmt_syntax *>())
        return ptr<syntax_tree_node>(result.as<assign_stmt_syntax *>());
    if (result.is<func_call_stmt_syntax *>())
        return ptr<syntax_tree_node>(result.as<func_call_stmt_syntax *>());
    if (result.is<block_syntax *>())
        return ptr<syntax_tree_node>(result.as<block_syntax *>());
    if (result.is<if_stmt_syntax *>())
        return ptr<syntax_tree_node>(result.as<if_stmt_syntax *>());
    if (result.is<while_stmt_syntax *>())
        return ptr<syntax_tree_node>(result.as<while_stmt_syntax *>());
    return nullptr;
}
