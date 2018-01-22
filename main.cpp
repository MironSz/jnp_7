#include <stack>
#include <string>
#include <functional>
#include <exception>
#include <cassert>
#include <map>
#include <list>
#include <iostream>

using Lazy = std::function<int(void)>;

class SyntaxError : public std::exception {
    const char *what() const noexcept { return "Ooops!\n"; }
};

class OperatorAlreadyDefined : public std::exception {
    const char *what() const noexcept { return "Ooops!\n"; }
};

class UnknownOperator : public std::exception {
    const char *what() const noexcept { return "Ooops!\n"; }
};

class LazyCalculator {
private:
    std::map<char, std::function<int(Lazy, Lazy)>> defined_operators;
public:
    Lazy parse(const std::string &s) const {
        std::stack<Lazy> stack_of_lazy;
        for (char c : s) {
            if (c == '2' || c == '4' || c == '0') {
                stack_of_lazy.push([c]() { return c - '0'; });
            } else {
                auto op = defined_operators.find(c);
                if (op == defined_operators.end()) {
                    throw UnknownOperator();
                }
                if (stack_of_lazy.size() < 2) {
                    throw SyntaxError();
                }
                Lazy a = stack_of_lazy.top();
                stack_of_lazy.pop();
                Lazy b = stack_of_lazy.top();
                stack_of_lazy.pop();
                auto f = std::bind(defined_operators.at(c), b, a);
                stack_of_lazy.push([f]() { return f(); });
            }
        }
        if (stack_of_lazy.size() != 1) {
            throw SyntaxError();
        }
        return stack_of_lazy.top();
    }

    int calculate(const std::string &s) const {
        return parse(s)();
    }

    void define(char c, std::function<int(Lazy, Lazy)> fn) {
        if (defined_operators.find(c) != defined_operators.end() || c == '2' || c == '4' || c == '0') {
            throw OperatorAlreadyDefined();
        }

        defined_operators.insert({c, fn});
    }

    LazyCalculator() {
        define('+', [](Lazy a, Lazy b) { return a() + b(); });
        define('-', [](Lazy a, Lazy b) { return a() - b(); });
        define('*', [](Lazy a, Lazy b) { return a() * b(); });
        define('/', [](Lazy a, Lazy b) { return a() / b(); });
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

    std::cout << buffer.length() << "    " << 42 * std::string("pomidor").length() << "\n";

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

