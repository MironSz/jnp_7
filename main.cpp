#include <stack>
#include <string>
#include <functional>
#include <exception>
#include <cassert>
#include <unordered_map>
#include <list>
#include <iostream>

using Lazy = std::function<int(void)>;

class SyntaxError : public std::exception {
    const char *what() const noexcept { return "Expression has uncorrect syntax\n"; }
};

class OperatorAlreadyDefined : public std::exception {
    const char *what() const noexcept { return "This operator is already defined\n"; }
};

class UnknownOperator : public std::exception {
    const char *what() const noexcept { return "This operator is not defined\n"; }
};

class LazyCalculator {
private:
    std::unordered_map<char, std::function<int(Lazy, Lazy)>> definedOperators;
public:
    Lazy parse(const std::string &s) const {
        std::stack<Lazy> stackOfLazy;
        for (char c : s) {
            if (c == '2' || c == '4' || c == '0') {
                Lazy l = [c]() { return c - '0'; };
                stackOfLazy.push(l);
            } else {
                auto op = definedOperators.find(c);
                if (op == definedOperators.end()) {
                    throw UnknownOperator();
                }
                if (stackOfLazy.size() < 2) {
                    throw SyntaxError();
                }
                Lazy a = stackOfLazy.top();
                stackOfLazy.pop();
                Lazy b = stackOfLazy.top();
                stackOfLazy.pop();
                auto f = std::bind(definedOperators.at(c), b, a);
                stackOfLazy.push(static_cast<Lazy>(f));
            }
        }
        if (stackOfLazy.size() != 1) {
            throw SyntaxError();
        }
        return stackOfLazy.top();
    }

    int calculate(const std::string &s) const {
        return parse(s)();
    }

    void define(char c, std::function<int(Lazy, Lazy)> fn) {
        if (definedOperators.find(c) != definedOperators.end()) {
            throw OperatorAlreadyDefined();
        }

        definedOperators.insert({c, fn});
    }

    LazyCalculator() {
        define('+', [](Lazy a, Lazy b) { return a() + b(); });
        define('-', [](Lazy a, Lazy b) { return a() - b(); });
        define('*', [](Lazy a, Lazy b) { return a() * b(); });
        define('/', [](Lazy a, Lazy b) { return a() / b(); });

        define('0', [](Lazy a, Lazy b) { return a() + b(); });
        define('2', [](Lazy a, Lazy b) { return a() + b(); });
        define('4', [](Lazy a, Lazy b) { return a() + b(); });
    }
};

std::function<void(void)> operator*(int n, std::function<void(void)> fn) {
    return [=]() {
        for (int i = 0; i < n; i++) {
            fn();
        }
    };
}

int manytimes(Lazy n, Lazy fn) {
    (n() * fn)();
    return 0;
}

int main() {
    LazyCalculator calculator;

    // The only literals...
    assert(calculator.calculate("0") == 0);
    assert(calculator.calculate("2") == 2);
    assert(calculator.calculate("4") == 4);
    // Built-in operators.
    assert(calculator.calculate("42+") == 6);
    assert(calculator.calculate("24-") == -2);
    assert(calculator.calculate("42*") == 8);
    assert(calculator.calculate("42/") == 2);

    assert(calculator.calculate("42-2-") == 0);
    assert(calculator.calculate("242--") == 0);
    assert(calculator.calculate("22+2-2*2/0-") == 2);

    // The fun.
    calculator.define('!', [](Lazy a, Lazy b) { return a() * 10 + b(); });
    assert(calculator.calculate("42!") == 42);

    std::string buffer;
    calculator.define(',', [](Lazy a, Lazy b) {
        a();
        return b();
    });
    calculator.define('P', [&buffer](Lazy, Lazy) {
        buffer += "pomidor";
        return 0;
    });
    assert(calculator.calculate("42P42P42P42P42P42P42P42P42P42P42P42P42P42P42P4"
                                        "2P,,,,42P42P42P42P42P,,,42P,42P,42P42P,,,,42P,"
                                        ",,42P,42P,42P,,42P,,,42P,42P42P42P42P42P42P42P"
                                        "42P,,,42P,42P,42P,,,,,,,,,,,,") == 0);


    assert(buffer.length() == 42 * std::string("pomidor").length());

    std::string buffer2 = std::move(buffer);
    buffer.clear();
    calculator.define('$', manytimes);
    assert(calculator.calculate("42!42P$") == 0);
    // Notice, how std::move worked.
    assert(buffer.length() == 42 * std::string("pomidor").length());

    calculator.define('?', [](Lazy a, Lazy b) { return a() ? b() : 0; });
    assert(calculator.calculate("042P?") == 0);
    assert(buffer == buffer2);

    assert(calculator.calculate("042!42P$?") == 0);
    assert(buffer == buffer2);

    calculator.define('1', [](Lazy, Lazy) { return 1; });
    assert(calculator.calculate("021") == 1);

    for (auto bad: {"", "42", "4+", "424+"}) {
        try {
            calculator.calculate(bad);
            assert(false);
        }
        catch (SyntaxError) {
        }
    }

    try {
        calculator.define('!', [](Lazy a, Lazy b) { return a() * 10 + b(); });
        assert(false);
    }
    catch (OperatorAlreadyDefined) {
    }

    try {
        calculator.define('0', [](Lazy, Lazy) { return 0; });
        assert(false);
    }
    catch (OperatorAlreadyDefined) {
    }

    try {
        calculator.calculate("02&");
        assert(false);
    }
    catch (UnknownOperator) {
    }

    return 0;
}

