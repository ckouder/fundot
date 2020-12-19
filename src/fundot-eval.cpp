/**
 * MIT License

 * Copyright (c) 2020 Jiacheng Huang

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "fundot-eval.h"

namespace fundot {
Object& Evaluator::temp(Object& obj)
{
    return eval(FunSetter(Symbol("__temp__"), FunQuote(obj)));
}

Object& Evaluator::temp(Object&& obj)
{
    return eval(FunSetter(Symbol("__temp__"), FunQuote(obj)));
}

Object& Evaluator::builtInQuit(FunList& fun_list)
{
    exit(EXIT_SUCCESS);
    return temp(fun_list);
}

Object& Evaluator::builtInIf(FunList& fun_list)
{
    FunList::Iterator iter = ++fun_list.begin();
    Object& predicate = eval(*iter);
    if (predicate.hasType<Null>() || predicate == Boolean(false)) {
        if (++iter != fun_list.end() && ++iter != fun_list.end()) {
            return *iter;
        }
        return temp(Null());
    }
    if (++iter != fun_list.end()) {
        return *iter;
    }
    return temp(fun_list);
}

Object& Evaluator::builtInCond(FunList& fun_list)
{
    FunList::Iterator iter = fun_list.begin();
    while (++iter != fun_list.end()) {
        Object& predicate = eval(*iter++);
        if (predicate.hasType<Null>() == false && predicate != Boolean(false)) {
            return *iter;
        }
    }
    return temp(Null());
}

Object& Evaluator::builtInWhile(FunList& fun_list)
{
    FunList::Iterator iter = ++fun_list.begin();
    FunList::Iterator predicate_iter = iter;
    Object& predicate = eval(*iter++);
    Object& to_eval = *iter;
    while (predicate.hasType<Null>() == false && predicate != Boolean(false)) {
        eval(to_eval);
        predicate = eval(*predicate_iter);
    }
    return temp(Null());
}

Object& Evaluator::builtInGet(FunList& fun_list)
{
    FunList::Iterator iter = ++fun_list.begin();
    if (iter->hasType<FunSet>()) {
        FunSet& fun_set = get<FunSet&>(*iter);
        FunSet::Iterator it = fun_set.find(FunSetter(*(++iter)));
        if (it != fun_set.end() && it->hasType<FunSetter>()) {
            return temp(get<FunSetter>(*it).value);
        }
    }
    if (iter->hasType<FunVector>()) {
        const FunVector& fun_vector = get<FunVector&>(*iter++);
        if (iter->hasType<Integer>()
            && static_cast<std::size_t>(get<Integer&>(*iter))
                   < fun_vector.size()) {
            return temp(fun_vector[get<Integer&>(*iter)]);
        }
        return temp(Null());
    }
    return temp(fun_list);
}

Object& Evaluator::builtInAdd(FunList& fun_list)
{
    FunList::Iterator iter = fun_list.begin();
    Float first = 0;
    if ((++iter)->hasType<Float>()) {
        first = get<Float>(*iter);
    }
    else if (iter->hasType<Integer>()) {
        first = get<Integer>(*iter);
    }
    Float second = 0;
    if ((++iter)->hasType<Float>()) {
        second = get<Float>(*iter);
    }
    else if (iter->hasType<Integer>()) {
        second = get<Integer>(*iter);
    }
    return temp(first + second);
}

Object& Evaluator::builtInMul(FunList& fun_list)
{
    FunList::Iterator iter = fun_list.begin();
    Float first = 0;
    if ((++iter)->hasType<Float>()) {
        first = get<Float>(*iter);
    }
    else if (iter->hasType<Integer>()) {
        first = get<Integer>(*iter);
    }
    Float second = 0;
    if ((++iter)->hasType<Float>()) {
        second = get<Float>(*iter);
    }
    else if (iter->hasType<Integer>()) {
        second = get<Integer>(*iter);
    }
    return temp(first * second);
}

Object& Evaluator::setValue(FunSet& fun_set, FunSetter& fun_setter)
{
    fun_setter.value = eval(fun_setter.value);
    fun_set.erase(fun_setter);
    fun_set.emplace(fun_setter);
    return get<const FunSetter&>(*fun_set.find(fun_setter)).value;
}

Object& Evaluator::getValue(FunSet& fun_set, const Object& key)
{
    FunSet::Iterator it = fun_set.find(FunSetter(key));
    if (it != fun_set.end() && it->hasType<FunSetter>()) {
        return get<const FunSetter&>(*it).value;
    }
    return temp(Null());
}

Object& Evaluator::operator()(Object& obj)
{
    return eval(obj);
}

Object& Evaluator::operator()(Object&& obj)
{
    return eval(obj);
}

Object& Evaluator::eval(FunQuote& fun_quote)
{
    return fun_quote.value;
}

Object& Evaluator::eval(FunSetter& fun_setter)
{
    fun_setter.value = eval(fun_setter.value);
    scope_.erase(fun_setter);
    scope_.emplace(fun_setter);
    return get<const FunSetter&>(*scope_.find(fun_setter)).value;
}

Object& Evaluator::eval(Symbol& symbol)
{
    FunSet::Iterator it = scope_.find(FunSetter(symbol));
    if (it != scope_.end() && it->hasType<FunSetter>()) {
        return eval(get<const FunSetter&>(*it).value);
    }
    return temp(symbol);
}

Object& Evaluator::eval(FunGetter& fun_getter)
{
    Object scope_ptr = &scope_;
    while (scope_ptr.hasType<FunSet*>()) {
        if (fun_getter.key.hasType<Symbol>()) {
            Object& value = getValue(*get<FunSet*>(scope_ptr), fun_getter.key);
            if (value.hasType<FunSet>() == false) {
                return temp(value);
            }
            scope_ptr = get<FunSet>(&value);
            if (fun_getter.value.hasType<Symbol>()) {
                return temp(
                    getValue(*get<FunSet*>(scope_ptr), fun_getter.value));
            }
            if (fun_getter.value.hasType<FunGetter>()) {
                fun_getter = fun_getter.value;
            }
            if (fun_getter.value.hasType<FunSetter>()) {
                return setValue(*get<FunSet*>(scope_ptr),
                                get<FunSetter&>(fun_getter.value));
            }
        }
    }
    return temp(fun_getter);
}

Object& Evaluator::eval(FunList& fun_list)
{
    static std::unordered_map<
        Object, std::function<Object&(Evaluator*, FunList&)>, Hash<Object>>
        built_in_macros = {{Symbol("quit"), &Evaluator::builtInQuit},
                           {Symbol("if"), &Evaluator::builtInIf},
                           {Symbol("cond"), &Evaluator::builtInCond},
                           {Symbol("while"), &Evaluator::builtInWhile}};
    if (built_in_macros.find(fun_list.front()) != built_in_macros.end()) {
        return eval(built_in_macros[fun_list.front()](this, fun_list));
    }
    FunList after_eval;
    FunList::Iterator it = fun_list.begin();
    while (it != fun_list.end()) {
        after_eval.pushBack(eval(*it++));
    }
    static std::unordered_map<
        Object, std::function<Object&(Evaluator*, FunList&)>, Hash<Object>>
        built_in_functions = {{Symbol("get"), &Evaluator::builtInGet},
                              {Symbol("add"), &Evaluator::builtInAdd},
                              {Symbol("mul"), &Evaluator::builtInMul}};
    if (built_in_functions.find(after_eval.front())
        != built_in_functions.end()) {
        return built_in_functions[after_eval.front()](this, after_eval);
    }
    return temp(after_eval);
}

Object& Evaluator::eval(FunVector& fun_vector)
{
    FunVector after_eval;
    for (Object& obj : fun_vector) {
        after_eval.pushBack(eval(obj));
    }
    return temp(after_eval);
}

Object& Evaluator::eval(Object& obj)
{
    if (obj.hasType<FunQuote>()) {
        return eval(get<FunQuote&>(obj));
    }
    if (obj.hasType<FunSetter>()) {
        return eval(get<FunSetter&>(obj));
    }
    if (obj.hasType<Symbol>()) {
        return eval(get<Symbol&>(obj));
    }
    if (obj.hasType<FunGetter>()) {
        return eval(get<FunGetter&>(obj));
    }
    if (obj.hasType<FunList>()) {
        return eval(get<FunList&>(obj));
    }
    if (obj.hasType<FunVector>()) {
        return eval(get<FunVector&>(obj));
    }
    return obj;
}

Object& Evaluator::eval(Object&& obj)
{
    Object to_eval(obj);
    return eval(to_eval);
}

}  // namespace fundot
