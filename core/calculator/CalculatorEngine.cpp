#include "CalculatorEngine.h"

#include <QStringView>
#include <cmath>
#include <cctype>
#include <limits>

namespace dante::calculator {

namespace {

// Recursive-descent parser. Grammar (PEMDAS, lowest → highest):
//   expr   := term   (('+'|'-') term)*
//   term   := factor (('*'|'/'|'%') factor)*
//   factor := unary  ('^' factor)?      // right-associative
//   unary  := ('+'|'-') unary | primary
//   primary:= number | '(' expr ')' | ident '(' expr ')' | ident
class Parser {
public:
    Parser(const QString& src, EvalResult& out) : s_(src), out_(out) {}

    bool parse(double& result) {
        skipWs();
        const double v = parseExpr();
        skipWs();
        if (!out_.ok) return false;
        if (pos_ < s_.size()) {
            fail(QStringLiteral("Unexpected character '%1'").arg(s_.at(pos_)));
            return false;
        }
        result = v;
        return true;
    }

private:
    const QString& s_;
    EvalResult&    out_;
    int            pos_{0};

    void fail(QString msg) {
        if (out_.ok) {  // keep the first error
            out_.ok = false;
            out_.error = std::move(msg);
        }
    }

    void skipWs() {
        while (pos_ < s_.size() && s_.at(pos_).isSpace()) ++pos_;
    }

    bool match(QChar c) {
        skipWs();
        if (pos_ < s_.size() && s_.at(pos_) == c) { ++pos_; return true; }
        return false;
    }

    QChar peek() const { return pos_ < s_.size() ? s_.at(pos_) : QChar(); }

    double parseExpr() {
        double left = parseTerm();
        while (out_.ok) {
            skipWs();
            const QChar c = peek();
            if (c == QLatin1Char('+')) {
                ++pos_;
                left += parseTerm();
            } else if (c == QLatin1Char('-')) {
                ++pos_;
                left -= parseTerm();
            } else break;
        }
        return left;
    }

    double parseTerm() {
        double left = parseFactor();
        while (out_.ok) {
            skipWs();
            const QChar c = peek();
            if (c == QLatin1Char('*')) {
                ++pos_;
                left *= parseFactor();
            } else if (c == QLatin1Char('/')) {
                ++pos_;
                const double r = parseFactor();
                if (r == 0.0) {
                    fail(QStringLiteral("Division by zero"));
                    return 0.0;
                }
                left /= r;
            } else if (c == QLatin1Char('%')) {
                ++pos_;
                const double r = parseFactor();
                if (r == 0.0) {
                    fail(QStringLiteral("Modulo by zero"));
                    return 0.0;
                }
                left = std::fmod(left, r);
            } else break;
        }
        return left;
    }

    double parseFactor() {
        const double left = parseUnary();
        skipWs();
        if (peek() == QLatin1Char('^')) {
            ++pos_;
            const double rhs = parseFactor(); // right-assoc
            return std::pow(left, rhs);
        }
        return left;
    }

    double parseUnary() {
        skipWs();
        if (peek() == QLatin1Char('-')) { ++pos_; return -parseUnary(); }
        if (peek() == QLatin1Char('+')) { ++pos_; return  parseUnary(); }
        return parsePrimary();
    }

    double parseNumber() {
        const int start = pos_;
        while (pos_ < s_.size() && (s_.at(pos_).isDigit() || s_.at(pos_) == QLatin1Char('.'))) ++pos_;
        // scientific notation: e/E followed by optional sign + digits
        if (pos_ < s_.size() && (s_.at(pos_) == QLatin1Char('e') || s_.at(pos_) == QLatin1Char('E'))) {
            ++pos_;
            if (pos_ < s_.size() && (s_.at(pos_) == QLatin1Char('+') || s_.at(pos_) == QLatin1Char('-'))) ++pos_;
            while (pos_ < s_.size() && s_.at(pos_).isDigit()) ++pos_;
        }
        const QStringView view(s_.constData() + start, pos_ - start);
        bool ok = false;
        const double v = view.toDouble(&ok);
        if (!ok) { fail(QStringLiteral("Invalid number")); return 0.0; }
        return v;
    }

    double parsePrimary() {
        skipWs();
        if (pos_ >= s_.size()) { fail(QStringLiteral("Unexpected end of expression")); return 0.0; }
        const QChar c = s_.at(pos_);
        if (c == QLatin1Char('(')) {
            ++pos_;
            const double v = parseExpr();
            if (!match(QLatin1Char(')'))) { fail(QStringLiteral("Missing ')'")); return 0.0; }
            return v;
        }
        if (c.isDigit() || c == QLatin1Char('.')) return parseNumber();
        if (c.isLetter()) {
            const int start = pos_;
            while (pos_ < s_.size() && (s_.at(pos_).isLetterOrNumber() || s_.at(pos_) == QLatin1Char('_'))) ++pos_;
            const QString name = s_.mid(start, pos_ - start).toLower();

            // Constants take precedence over zero-arg functions.
            if (name == QStringLiteral("pi"))  return M_PI;
            if (name == QStringLiteral("e"))   return M_E;

            skipWs();
            if (!match(QLatin1Char('('))) {
                fail(QStringLiteral("Unknown identifier '%1'").arg(name));
                return 0.0;
            }
            const double arg = parseExpr();
            if (!match(QLatin1Char(')'))) { fail(QStringLiteral("Missing ')'")); return 0.0; }
            return applyFunction(name, arg);
        }
        fail(QStringLiteral("Unexpected character '%1'").arg(c));
        return 0.0;
    }

    double applyFunction(const QString& name, double arg) {
        if (name == QStringLiteral("sin"))   return std::sin(arg);
        if (name == QStringLiteral("cos"))   return std::cos(arg);
        if (name == QStringLiteral("tan"))   return std::tan(arg);
        if (name == QStringLiteral("sqrt"))  {
            if (arg < 0) { fail(QStringLiteral("sqrt of negative")); return 0.0; }
            return std::sqrt(arg);
        }
        if (name == QStringLiteral("log"))   {
            if (arg <= 0) { fail(QStringLiteral("log of non-positive")); return 0.0; }
            return std::log10(arg);
        }
        if (name == QStringLiteral("ln"))    {
            if (arg <= 0) { fail(QStringLiteral("ln of non-positive")); return 0.0; }
            return std::log(arg);
        }
        if (name == QStringLiteral("abs"))   return std::fabs(arg);
        if (name == QStringLiteral("floor")) return std::floor(arg);
        if (name == QStringLiteral("ceil"))  return std::ceil(arg);
        if (name == QStringLiteral("round")) return std::round(arg);
        fail(QStringLiteral("Unknown function '%1'").arg(name));
        return 0.0;
    }
};

} // namespace

CalculatorEngine::CalculatorEngine() = default;

EvalResult CalculatorEngine::evaluate(const QString& expression) {
    EvalResult r;
    r.ok = true;
    if (expression.trimmed().isEmpty()) {
        r.ok = false;
        r.error = QStringLiteral("Empty expression");
        return r;
    }
    Parser p(expression, r);
    double v = 0.0;
    if (!p.parse(v)) {
        r.value = 0.0;
        return r;
    }
    if (std::isnan(v)) { r.ok = false; r.error = QStringLiteral("Result is NaN"); return r; }
    if (std::isinf(v)) { r.ok = false; r.error = QStringLiteral("Result is infinite"); return r; }
    r.value = v;
    pushHistory(expression.trimmed(), v);
    return r;
}

void CalculatorEngine::pushHistory(const QString& expr, double value) {
    history_.push_back(HistoryEntry{expr, value});
    while (static_cast<int>(history_.size()) > kHistoryCap) history_.pop_front();
}

void CalculatorEngine::setHistory(std::deque<HistoryEntry> entries) {
    history_ = std::move(entries);
    while (static_cast<int>(history_.size()) > kHistoryCap) history_.pop_front();
}

} // namespace dante::calculator
