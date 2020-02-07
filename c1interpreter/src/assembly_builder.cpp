
#include "assembly_builder.h"

#include <vector>
#define GetIntConst(x) ConstantInt::get(Type::getInt32Ty(context),x)
#define GetFloatConst(x) ConstantFP::get(Type::getDoubleTy(context),x)

using namespace llvm;
using namespace c1_recognizer::syntax_tree;

void assembly_builder::visit(assembly &node)
{
    for(auto &child:node.global_defs){
        in_global = true;
        value_result=GetIntConst(0);
        lval_as_rval=true;
        constexpr_expected=false;
        child->accept(*this);
    }
}
//check
void assembly_builder::visit(func_def_syntax &node)
{
    in_global=false;
    if(functions.count(node.name)!=0){
        err.error(node.line,node.pos,"Multiple Definition of Function:"+node.name);
        error_flag=true;
        return;
    }
    current_function = Function::Create(FunctionType::get(Type::getVoidTy(context), {}, false)
                                        , GlobalValue::LinkageTypes::ExternalLinkage,
                                        node.name, module.get());
                                        //只有void类返回值
    functions.emplace(node.name,current_function);
    auto Entry = BasicBlock::Create(context, "Entry", current_function);
    builder.SetInsertPoint(Entry);
    node.body->accept(*this);
    builder.CreateRetVoid();
    //只有void类返回值
}
//check
void assembly_builder::visit(cond_syntax &node)
{
    node.lhs->accept(*this);
    auto lhs=value_result;
    auto lhs_type=is_result_int;
    if(value_result==nullptr){
        return;
    }
    node.rhs->accept(*this);
    auto rhs=value_result;
    auto rhs_type=is_result_int;
    if(value_result==nullptr){
        return;
    }
    is_result_int=true;
    if(!(lhs_type && rhs_type)){
        if(lhs_type){
            lhs=builder.CreateSIToFP(lhs,Type::getDoubleTy(context));
        }
        if(rhs_type){
            rhs=builder.CreateSIToFP(rhs,Type::getDoubleTy(context));
        }
        is_result_int=false;
    }
    //std::cout<<"lhstype:"+std::to_string(lhs_type)+'\n'+"rhstype"+std::to_string(rhs_type)+'\n';
    if(node.op==relop::equal){
        if(!is_result_int){
            err.error(node.line,node.pos,"Cannot use float number in == cond.");
            error_flag=true;
            value_result=nullptr;
            if(constexpr_expected){
                    int_const_result=0;
                    float_const_result=0;
                }
            return;
        }
        if(is_result_int)
            value_result=builder.CreateICmpEQ(lhs,rhs);
    }
    if(node.op==relop::non_equal){
        if(!is_result_int){
            err.error(node.line,node.pos,"Cannot use float number in == cond.");
            error_flag=true;
            value_result=nullptr;
            if(constexpr_expected){
                    int_const_result=0;
                    float_const_result=0;
                }
            return;
        }
        value_result=builder.CreateICmpNE(lhs,rhs);
    }
    if(node.op==relop::greater){
        if(is_result_int)
            value_result=builder.CreateICmpSGT(lhs,rhs);
        else
            value_result=builder.CreateFCmpOGT(lhs,rhs);
    }
    if(node.op==relop::greater_equal){
        if(is_result_int)
            value_result=builder.CreateICmpSGE(lhs,rhs);
        else
            value_result=builder.CreateFCmpOGE(lhs,rhs);
    }
    if(node.op==relop::less){
        if(is_result_int)
            value_result=builder.CreateICmpSLT(lhs,rhs);
        else
            value_result=builder.CreateFCmpOLT(lhs,rhs);        
    }
    if(node.op==relop::less_equal){
        if(is_result_int)
            value_result=builder.CreateICmpSLE(lhs,rhs);
        else
            value_result=builder.CreateFCmpOLE(lhs,rhs);        
    }
    
}
//check
void assembly_builder::visit(binop_expr_syntax &node)
{
    node.lhs->accept(* this);
    if(value_result==nullptr)
        return;
    auto lhs = value_result;
    bool lhs_type=is_result_int;
    int lhs_int=int_const_result;
    double lhs_float=float_const_result;
    node.rhs->accept(* this);
    if(value_result==nullptr)
        return;
    auto rhs = value_result;
    bool rhs_type=is_result_int;
    int rhs_int=int_const_result;
    double rhs_float=float_const_result;
    if(lhs_type&&rhs_type){
        is_result_int=true;
    }
    else{
        is_result_int=false;
    }
    //如果左右操作数有一个为float，就要强制类型转换了。
    if(node.op==binop::plus){
        if(constexpr_expected){
            if(is_result_int){
                int_const_result=lhs_int+rhs_int;
            }
            else{
                double x;
                double y;
                if(lhs_type)x=lhs_int;
                else x=lhs_float;
                if(rhs_type)y=rhs_int;
                else y=rhs_float;
                float_const_result=double(x+y);
            }
        }
        else{
            if(!is_result_int){
                if(lhs_type){
                    lhs=builder.CreateSIToFP(lhs,Type::getDoubleTy(context));
                }
                if(rhs_type){
                    rhs=builder.CreateSIToFP(rhs,Type::getDoubleTy(context));
                }
            }
            if(is_result_int)
                value_result=builder.CreateAdd(lhs,rhs);
            else
                value_result=builder.CreateFAdd(lhs,rhs);
        }
    }
    else if(node.op==binop::minus){
        if(constexpr_expected){
            if(is_result_int){
                int_const_result=lhs_int-rhs_int;
            }
            else{
                double x;
                double y;
                if(lhs_type)x=lhs_int;
                else x=lhs_float;
                if(rhs_type)y=rhs_int;
                else y=rhs_float;
                float_const_result=double(x-y);
            }
        }
        else{
            if(!is_result_int){
                if(lhs_type){
                    lhs=builder.CreateSIToFP(lhs,Type::getDoubleTy(context));
                }
                if(rhs_type){
                    rhs=builder.CreateSIToFP(rhs,Type::getDoubleTy(context));
                }
            }
            if(is_result_int)
                value_result=builder.CreateSub(lhs,rhs);
            else
                value_result=builder.CreateFSub(lhs,rhs);
        }
    }
    else if(node.op==binop::multiply){
        if(constexpr_expected){
            if(is_result_int){
                int_const_result=lhs_int*rhs_int;
            }
            else{
                double x;
                double y;
                if(lhs_type)x=lhs_int;
                else x=lhs_float;
                if(rhs_type)y=rhs_int;
                else y=rhs_float;
                float_const_result=double(x*y);
            }
        }
        else{
            if(!is_result_int){
                if(lhs_type){
                    lhs=builder.CreateSIToFP(lhs,Type::getDoubleTy(context));
                }
                if(rhs_type){
                    rhs=builder.CreateSIToFP(rhs,Type::getDoubleTy(context));
                }
            }
            if(is_result_int)
                value_result=builder.CreateMul(lhs,rhs);
            else
                value_result=builder.CreateFMul(lhs,rhs);
        }
    }
    else if(node.op==binop::divide){
        if(constexpr_expected==true){//常量情形
            if(is_result_int){
                if(rhs_int==0){
                    err.error(node.pos,node.line,"Divide by 0 error, Int Case");
                    error_flag=true;
                    value_result=nullptr;
                    if(constexpr_expected){
                    int_const_result=0;
                    float_const_result=0;
                    }
                    return;
                }
                int_const_result=lhs_int/rhs_int;
            }
            else{
                double x;
                double y;
                if(lhs_type)x=lhs_int;
                else x=lhs_float;
                if(rhs_type)y=rhs_int;
                else y=rhs_float;
                if(std::abs(y)<1e-5){
                    err.error(node.pos,node.line,"Divide by 0 error, Double Case");
                    error_flag=true;
                    value_result=nullptr;
                    if(constexpr_expected){
                    int_const_result=0;
                    float_const_result=0;
                    }
                    return;
                }
                float_const_result=double(x/y);
            }
        }
        else{
            //非常量情形
            if(!is_result_int){
                if(lhs_type){
                    lhs=builder.CreateSIToFP(lhs,Type::getDoubleTy(context));
                }
                if(rhs_type){
                    rhs=builder.CreateSIToFP(rhs,Type::getDoubleTy(context));
                }
            }
            if(is_result_int)
                value_result = builder.CreateSDiv(lhs, rhs);
            else
                value_result = builder.CreateFDiv(lhs, rhs);
        }
    }
    else if(node.op==binop::modulo){
        if(constexpr_expected==true){//常量情形
            if(is_result_int){
                if(rhs_int==0){
                    err.error(node.pos,node.line,"Divide by 0 error, Int Case");
                    error_flag=true;
                    value_result=nullptr;
                    int_const_result=0;
                    float_const_result=0;
                    return;
                }
                int_const_result=lhs_int%rhs_int;
            }
            else{
                err.error(node.pos,node.line,"Cannot do modulo on Float num");
                error_flag=true;
                value_result=nullptr;
                int_const_result=0;
                float_const_result=0;
                return;
                /*double x;
                double y;
                if(lhs_type)x=lhs_int;
                else x=lhs_float;
                if(rhs_type)y=rhs_int;
                else y=rhs_float;
                if(std::abs(y)<1e-5){
                    err.error(node.pos,node.line,"Divide by 0 error, Double Case");
                    error_flag=true;
                    value_result=nullptr;
                    int_const_result=0;
                    float_const_result=0;
                    return;
                }
                float_const_result=double(x%y);*/
            }
        }
        else{
            //非常量情形
            if(!is_result_int){
                err.error(node.pos,node.line,"Cannot do modulo on Float num");
                error_flag=true;
                value_result=nullptr;
                return;
                /*if(lhs_type){
                    lhs=builder.CreateSIToFP(lhs,Type::getDoubleTy(context));
                }
                else if(rhs_type){
                    rhs=builder.CreateSIToFP(rhs,Type::getDoubleTy(context));
                }*/
            }
            value_result = builder.CreateSRem(lhs, rhs);
        }
    }
}
//check
void assembly_builder::visit(unaryop_expr_syntax &node)
{
    node.rhs->accept(*this);
    if(value_result==nullptr)return;
    if(constexpr_expected==true){    
        if(is_result_int==true){
            if(node.op==unaryop::minus)
                int_const_result=-int_const_result;
        }
        else{
            if(node.op==unaryop::minus)
                float_const_result=-float_const_result;
        }
    }
    else{
        if(node.op==unaryop::minus){
            if(is_result_int)
                value_result=builder.CreateNeg(value_result);
            else
                value_result=builder.CreateFNeg(value_result);
        }
    }
}
//check
void assembly_builder::visit(lval_syntax &node)
{
    //首先检查这个变量是否定义了。
    if(std::get<0>(lookup_variable(node.name))==nullptr){
        err.error(node.pos,node.line,"Undeclared varaible refered:"+node.name);
        error_flag=true;
            if(constexpr_expected){
                int_const_result=0;
                float_const_result=0;
            }
        value_result=nullptr;
        return;
    }
    if(lval_as_rval){
        //左值作为右值使用
        if(constexpr_expected==true){
            //变量（用作右值的左值）不能在constexpr里出现
            err.error(node.pos,node.line,"Cannot use variables in constexpr:"+node.name);
            value_result=nullptr;
            if(constexpr_expected){
                    int_const_result=0;
                    float_const_result=0;
                }
            error_flag=true;
        }
        if(node.array_index==nullptr){
            //非数组情形
            if(std::get<2>(lookup_variable(node.name))==true){
                //如果这里的lval是个数组（指针值），因为不知道具体要使用它的什么值
                //（它的指针值我们又不知道，不能按照C语言的风格进行运算，所以我按照它是错误处理了）
                err.error(node.pos,node.line,"Cannot refer an array as rval:"+node.name);
                value_result=nullptr;
                if(constexpr_expected){
                    int_const_result=0;
                    float_const_result=0;
                }
                error_flag=true;
                return;
            }
            //否则，可以直接引用
            auto lval_ptr=std::get<0>(lookup_variable(node.name));
            if(std::get<3>(lookup_variable(node.name))==true){
                value_result = builder.CreateLoad(Type::getInt32Ty(context), lval_ptr);
                is_result_int=true;
            }
            else{
                value_result = builder.CreateLoad(Type::getDoubleTy(context), lval_ptr);
                is_result_int=false;
            }
            //类型检查
        }
        else{
            //数组索引情形，要额外访问index
            if(std::get<2>(lookup_variable(node.name))==false){
                //不能对非数组进行索引
                err.error(node.pos,node.line,"Cannot index an non-array:"+node.name);
                value_result=nullptr;
                if(constexpr_expected){
                    int_const_result=0;
                    float_const_result=0;
                }
                error_flag=true;
                return;
            }
            node.array_index->accept(*this);
            if(is_result_int==false){
                //index必须是整型
                err.error(node.pos,node.line,"Cannot index an array with float number:"+node.name);
                value_result=nullptr;
                if(constexpr_expected){
                    int_const_result=0;
                    float_const_result=0;
                }
                error_flag=true;
                return;
            }
            Value* indexList[] = {GetIntConst(0),  value_result};
            // 从0到value_result偏移寻址
            auto lval_ptr=std::get<0>(lookup_variable(node.name));
            auto tmp2=builder.CreateGEP(lval_ptr,indexList);
            if(std::get<3>(lookup_variable(node.name))==true){
                value_result=builder.CreateLoad(Type::getInt32Ty(context),tmp2);
                is_result_int=true;
            }
            else{
                value_result=builder.CreateLoad(Type::getDoubleTy(context),tmp2);
                is_result_int=false;
            }
        }
    }
    else{
        //左值作为左值来使用
        if (node.array_index == nullptr) {
			value_result = std::get<0>(lookup_variable(node.name));
            is_result_int=std::get<3>(lookup_variable(node.name));
            //
            //std::cout<<node.name+",result_init:"+std::to_string(is_result_int);
            //
        }
		else{
            if(std::get<2>(lookup_variable(node.name))==false){
                //不能对非数组进行索引
                err.error(node.pos,node.line,"Cannot index an non-array:"+node.name);
                value_result=nullptr;
                if(constexpr_expected){
                    int_const_result=0;
                    float_const_result=0;
                }
                error_flag=true;
                return;
            }
			auto tmp_constexpr_expected = constexpr_expected;
			constexpr_expected = false;
            //这里是stmt_syntax的子类，不会出现需要常量表达式的情形。但是为了不影响已有的状态，暂存这个flag值。
			auto tmp_lval = lval_as_rval;
			lval_as_rval = true;
            //后面如果有左值的话，就把它当做右值来用。
            //C1Parser的定义决定不会有index为赋值语句的情形。
			node.array_index->accept(*this);
			if (value_result == nullptr) {
				return;
                //value_result是nullptr只会对应出错的情形，跳过这颗子树。
			}
            //因为前面已经把constexpr_expected改成了false,这里即使访问literal_syntax也会返回一个存着固定数字的变量，放在value_result里。
            if(is_result_int==false){
                //index必须是整型
                err.error(node.pos,node.line,"Cannot index an array with float number:"+node.name);
                value_result=nullptr;
                if(constexpr_expected){
                    int_const_result=0;
                    float_const_result=0;
                }
                error_flag=true;
                return;
            }
			lval_as_rval = tmp_lval;
			constexpr_expected = tmp_constexpr_expected;
			Value* indexList[] = { GetIntConst(0), value_result };
			//偏移寻址
            auto id_ptr = std::get<0>(lookup_variable(node.name));
			value_result = builder.CreateGEP(id_ptr, indexList);
            is_result_int=std::get<3>(lookup_variable(node.name));
		}
    }
}
//check
void assembly_builder::visit(literal_syntax &node)
{
    if (constexpr_expected == false) {
		// value_result
        // constexpr_expected==true也可能走到这一步（即使这就是一个const值），为了让后面能得到一个可用的值，要分类。
        if(node.is_int==true){
		    value_result = ConstantInt::get(Type::getInt32Ty(context), node.intConst);
            is_result_int=true;            
        }
        else{
            value_result = ConstantFP::get(Type::getDoubleTy(context), node.floatConst);
            is_result_int=false;               
        }
	}
	else {
		// constexpr_expected
        if(node.is_int==true){
		    int_const_result = node.intConst;
            is_result_int=true;            
        }
        else{
            is_result_int=false;            
            float_const_result=node.floatConst;
        }
	}
}
//check
void assembly_builder::visit(var_def_stmt_syntax &node)
{
    if(variables[0].count(node.name)){
        //重复定义
        //观察到exit_scope()和enter_scope()的实现，用lookup这种跨越多个scope的方法不能正确判断，
        //因为enter是emplace多了一层variables，exit则是pop出，所以就要在栈顶层搜索才能正确地维护作用域。
        err.error(node.line,node.pos,"Cannot declare an ID multiple times!:"+node.name);
        error_flag=true;
        value_result=nullptr;
        if(constexpr_expected==true){
          int_const_result=0;
           float_const_result=0;
        }
        return ;
    }
    if(in_global){
        //全局变量
        constexpr_expected=true;
        //初始化表达式要求为常量表达式
        if(node.array_length==nullptr){
            //非数组
            if(node.is_int==true){
                llvm::ConstantInt *tmp_int_const_result;
                //常量非数组变量
                if(node.initializers.size()==0){
                    //未初始化的全局变量
                     tmp_int_const_result=GetIntConst(0);
                     //初始化为默认值0
                     auto dec_int_const_global = new GlobalVariable(*module.get(), Type::getInt32Ty(context), node.is_constant, GlobalValue::LinkageTypes::ExternalLinkage, tmp_int_const_result, node.name);
                    declare_variable(node.name, dec_int_const_global, node.is_constant, node.array_length != nullptr,node.is_int);
                }
                else{
                    node.initializers[0]->accept(*this);
                    if(value_result==nullptr)
                        return;
                    //错误检测，直接抛弃结果
                    if(is_result_int==false){
                        //因为是常量表达式，直接把值变成int就好了
                        tmp_int_const_result=GetIntConst((int)float_const_result);
                    }
                    else{
                        tmp_int_const_result=GetIntConst(int_const_result);
                    }
                    auto dec_int_const_global = new GlobalVariable(*module.get(), Type::getInt32Ty(context), node.is_constant, GlobalValue::LinkageTypes::ExternalLinkage, tmp_int_const_result, node.name);
                    declare_variable(node.name, dec_int_const_global, node.is_constant, node.array_length != nullptr,node.is_int);
                }
            }
            else{
                llvm::Constant *tmp_float_const_result;
                //常量非数组变量
                if(node.initializers.size()==0){
                    //未初始化的全局变量
                     tmp_float_const_result=GetFloatConst(0);
                     //初始化为默认值0
                     auto dec_float_const_global = new GlobalVariable(*module.get(), Type::getDoubleTy(context), node.is_constant, GlobalValue::LinkageTypes::ExternalLinkage, tmp_float_const_result, node.name);
                    declare_variable(node.name, dec_float_const_global, node.is_constant, node.array_length != nullptr,node.is_int);
                }
                else{
                    node.initializers[0]->accept(*this);
                    if(value_result==nullptr)
                        return;
                    //错误检测，直接抛弃结果
                    if(is_result_int==true){
                        //因为是常量表达式，直接把值变成double就好了
                        tmp_float_const_result=GetFloatConst((double)int_const_result);
                    }
                    else{
                        tmp_float_const_result=GetFloatConst(float_const_result);
                    }
                    auto dec_float_const_global = new GlobalVariable(*module.get(), Type::getDoubleTy(context), node.is_constant, GlobalValue::LinkageTypes::ExternalLinkage, tmp_float_const_result, node.name);
                    declare_variable(node.name, dec_float_const_global, node.is_constant, node.array_length != nullptr,node.is_int);
                }
            }
        }
        else{
            //数组
            //先无视已有的错误，直接把数组读完再决定是否有错误。
            bool temp_error_flag=error_flag;
            error_flag=false;
            lval_as_rval=true;
            node.array_length->accept(*this);
            int length;
            if(is_result_int==false){
                err.error(node.line,node.pos,"Cannot index an array with float num!:"+std::to_string(float_const_result));
                error_flag=true;
            }
            length=int_const_result;
            if(length<node.initializers.size()){
                err.error(node.line,node.pos,"Array overflow:"+node.name);
                error_flag=true;
            }
            if(length<=0){
                err.error(node.line,node.pos,"Array must be larger than 1:"+node.name);
                error_flag=true;
            }
            std::vector<Constant *> tmp_vec;
            for(auto &tmp_result:node.initializers){
                lval_as_rval=true;
                constexpr_expected=true;
                //为了防止错误的情况下把这些值修改了。
                //这里先不处理错误
                tmp_result->accept(*this);
                if(node.is_int==true){
                    if(is_result_int==true)
                        tmp_vec.push_back(GetIntConst(int_const_result));
                    else
                        tmp_vec.push_back(GetIntConst((int)float_const_result));
                }
                else{
                    if(is_result_int==false)
                        tmp_vec.push_back(GetFloatConst(float_const_result));
                    else
                        tmp_vec.push_back(GetFloatConst((double)int_const_result));
                }
            }
            for(int i=tmp_vec.size();i<length;i++){
                if(node.is_int==true)
                    tmp_vec.push_back(GetIntConst(0));
                else
                    tmp_vec.push_back(GetFloatConst(0));
            }
            if(error_flag==true){
                return;
            }
            else{
                error_flag=temp_error_flag;
            }
            if(node.is_int==true){
                auto tmp_array_global =ConstantArray::get(ArrayType::get(Type::getInt32Ty(context), length), tmp_vec); 
                auto dec_int_const_array_global = new GlobalVariable(*module.get(), ArrayType::get(Type::getInt32Ty(context), length),
                                                                        node.is_constant, GlobalValue::LinkageTypes::ExternalLinkage, tmp_array_global, node.name);
                declare_variable(node.name,dec_int_const_array_global , node.is_constant, node.array_length != nullptr,node.is_int);
            }
            else{
                auto tmp_array_global =ConstantArray::get(ArrayType::get(Type::getDoubleTy(context), length), tmp_vec); 
                auto dec_float_const_array_global = new GlobalVariable(*module.get(), ArrayType::get(Type::getDoubleTy(context), length),
                                                                        node.is_constant, GlobalValue::LinkageTypes::ExternalLinkage, tmp_array_global, node.name);
                declare_variable(node.name,dec_float_const_array_global , node.is_constant, node.array_length != nullptr,node.is_int);
            }
        }
        is_result_int=node.is_int;
    }
    else{
        //局部变量
        if(node.array_length==nullptr){
            constexpr_expected=false;
            Value * result;
            if(node.is_int){
                result = builder.CreateAlloca(Type::getInt32Ty(context), 
                                                nullptr, node.name);
            }
            else{
                result = builder.CreateAlloca(Type::getDoubleTy(context), 
                                                nullptr, node.name);
            }
            if(node.initializers.size()!=0){
                node.initializers[0]->accept(*this);
                if(value_result==nullptr)
                    return;
                if(node.is_int){
                    if(is_result_int==false){
                        value_result=builder.CreateFPToSI(value_result,Type::getInt32Ty(context));
                    }
                    is_result_int=true;
                }
                else{
                    if(is_result_int==true){
                        value_result=builder.CreateSIToFP(value_result,Type::getDoubleTy(context));                        
                    }
                    is_result_int=false;
                }
                builder.CreateStore(value_result,result);
            }
            declare_variable(node.name, result, node.is_constant, node.array_length != nullptr,node.is_int);
        }
        else{
            //数组
            //和全局变量的思路是一样的
            constexpr_expected = true;
			auto temp_error_flag = error_flag;
			error_flag = false;
			node.array_length->accept(*this);
            int length;
            constexpr_expected=false;
            if(is_result_int==false){
                err.error(node.line,node.pos,"Cannot index an array with float num!:"+std::to_string(float_const_result));
                error_flag=true;
            }
            length=int_const_result;
            if(length<node.initializers.size()){
                err.error(node.line,node.pos,"Array overflow:"+node.name);
                error_flag=true;
            }
            if(length<=0){
                err.error(node.line,node.pos,"Array must be larger than 1:"+node.name);
                error_flag=true;
            }
			if (error_flag) {
				return;
			}
			else {
				error_flag = temp_error_flag;
			}
            Value * tmp_result2;
			if(node.is_int){
                tmp_result2 = builder.CreateAlloca(ArrayType::get(Type::getInt32Ty(context), length), GetIntConst(length), node.name);
            }
            else{
                tmp_result2 = builder.CreateAlloca(ArrayType::get(Type::getDoubleTy(context), length),  GetIntConst(length), node.name);
            }
			int i = 0;
            Value * index;
			for (auto &init : node.initializers) {
				Value* indexList[] = {GetIntConst(0), GetIntConst(i++) };
				index = builder.CreateGEP(tmp_result2, indexList);
                //取得初始化因子在数组中的位置对应的地址
				init->accept(*this);
                if(is_result_int!=node.is_int){
                    if(node.is_int){
                        value_result=builder.CreateFPToSI(value_result,Type::getInt32Ty(context));
                    }
                    else{
                        value_result=builder.CreateSIToFP(value_result,Type::getDoubleTy(context));
                    }
                }
                builder.CreateStore(value_result, index);            

			}
            is_result_int=node.is_int;
            declare_variable(node.name, tmp_result2, node.is_constant, node.array_length != nullptr,node.is_int);
        }
    }
    is_result_int=node.is_int;
}
//check
void assembly_builder::visit(assign_stmt_syntax &node)
{
    if (std::get<1>(lookup_variable(node.target->name)) == true) {
		// 被赋值数是Const
		err.error(node.target->line, node.target->pos, "Cannot make assignment to const:" + node.target->name);
		error_flag = true;
		return;
	}
	else if (std::get<0>(lookup_variable(node.target->name)) == nullptr) {
		// 使用未定义变量
		err.error(node.target->line, node.target->pos, "Cannot refer undeclared variable:" + node.target->name);
		error_flag = true;
		return;
	}
    else if (std::get<2>(lookup_variable(node.target->name)) == true && node.target->array_index == nullptr) {
		// 对数组名字直接赋值
		err.error(node.target->line, node.target->pos, "Canot assign to an array name:" + node.target->name);
		error_flag = true;
		return;
	}
	else if (std::get<2>(lookup_variable(node.target->name)) == false && node.target->array_index ) {
		// 对非数组取下标
		err.error(node.target->line, node.target->pos, "Cannot index a non-array:" + node.target->name);
		error_flag = true;
		return;
	}
    lval_as_rval=false;
    // 左值用作左值
    node.target->accept(* this);
    auto tmp1=value_result;
    lval_as_rval=true;
    auto rtmp1=is_result_int;
    // 没有连等
    node.value->accept(* this);
    auto tmp2=value_result;
    auto rtmp2=is_result_int;
    value_result=GetIntConst(0);
    //因为C1没有连等的情形，也不能使用赋值语句的返回值，所以使用int 0作为一个默认的返回值。
	if(tmp1==nullptr || tmp2==nullptr){
        return;
        //从出错子树回退
    }
    //std::cout<<" assign ,rtmp1:"+std::to_string(rtmp1)+" "+"rtmp2:"+
    //std::to_string(rtmp2);
    Value* final_tmp;
    if(rtmp1!=rtmp2){
        //强制类型转换
        if(!rtmp2){
            final_tmp=builder.CreateFPToSI(tmp2,Type::getInt32Ty(context));
            //std::cout<<" zhixingle!fptosi";
        }
        else{
            final_tmp=builder.CreateSIToFP(tmp2,Type::getDoubleTy(context));
            //std::cout<<" zhixingle! sitofp";
        }
    }
    else{
        final_tmp=tmp2;
    }
    builder.CreateStore(final_tmp, tmp1);
    is_result_int=rtmp1;
}
//check
void assembly_builder::visit(func_call_stmt_syntax &node)
{
    auto fcalled=functions[node.name];
    if(fcalled == nullptr){
        error_flag=true;
        err.error(node.line,node.pos,"Called function do not exits:"+node.name);
        return;
    }
    builder.CreateCall(fcalled,{});
}
//check
void assembly_builder::visit(block_syntax &node)
{
    enter_scope();
    for(auto &tmp:node.body){
        value_result=GetIntConst(0);
        lval_as_rval=true;
        constexpr_expected=false;
        tmp.get()->accept(* this);
    }
    exit_scope();
}
//check
void assembly_builder::visit(if_stmt_syntax &node)
{
    auto condition_block = BasicBlock::Create(context,"Cond_"+
    std::to_string(node.pos)+"_"+std::to_string(node.line),current_function);

    //cond
    builder.CreateBr(condition_block);
    builder.SetInsertPoint(condition_block);
    node.pred->accept(*this);

    auto then_block=BasicBlock::Create(context,"Then_"+
    std::to_string(node.pos)+"_"+std::to_string(node.line),current_function);
    auto else_block=BasicBlock::Create(context,"Else_"+
    std::to_string(node.pos)+"_"+std::to_string(node.line),current_function);
    auto next_block=BasicBlock::Create(context,"Next_"+
    std::to_string(node.pos)+"_"+std::to_string(node.line),current_function);
    //在then_block中要跳转到next_block,在else_block中也要跳转到next_block
    //即使else_body不存在，这里也可以创建一个else_body,只是没有访问者方法的调用

    builder.CreateCondBr(value_result,then_block,else_block);
    //then
    builder.SetInsertPoint(then_block);
    node.then_body->accept(*this);
    builder.CreateBr(next_block);

    //else
    builder.SetInsertPoint(else_block);
    if(node.else_body){
        node.else_body->accept(*this);
    }
    builder.CreateBr(next_block);

    //next
    builder.SetInsertPoint(next_block);
}
//check
void assembly_builder::visit(while_stmt_syntax &node)
{
    auto loopcondition_block = BasicBlock::Create(context,"LoopCond_"+
    std::to_string(node.pos)+"_"+std::to_string(node.line),current_function);
    auto loopcontent_block=BasicBlock::Create(context,"LoopContent_"+
    std::to_string(node.pos)+"_"+std::to_string(node.line),current_function);
    auto afterloop_block=BasicBlock::Create(context,"AfterLoop_"+
    std::to_string(node.pos)+"_"+std::to_string(node.line),current_function);

    builder.CreateBr(loopcondition_block);
    builder.SetInsertPoint(loopcondition_block);
    node.pred->accept(*this);

    builder.CreateCondBr(value_result,loopcontent_block,afterloop_block);
    builder.SetInsertPoint(loopcontent_block);
    if(node.body){
        node.body->accept(*this);
    }
    builder.CreateBr(loopcondition_block);

    builder.SetInsertPoint(afterloop_block);

}
//check
void assembly_builder::visit(empty_stmt_syntax &node)
{
    return;
}
//no need to check