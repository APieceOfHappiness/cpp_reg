#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <functional>

struct Node {
    char type;
    unsigned cp = 0;
    int a = -1, b = -1;
};

std::vector<Node> pool;

int makeNode(char type, unsigned cp = 0, int a = -1, int b = -1) {
    pool.push_back({type, cp, a, b});
    return (int)pool.size() - 1;
}

std::vector<unsigned> decode(const std::string& s) {
    std::vector<unsigned> res;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = s[i];
        unsigned cp; int len;
        if (c < 0x80)       { cp = c;        len = 1; }
        else if (c >= 0xF0) { cp = c & 0x07; len = 4; }
        else if (c >= 0xE0) { cp = c & 0x0F; len = 3; }
        else                { cp = c & 0x1F; len = 2; }
        for (int k = 1; k < len; k++) cp = (cp << 6) | (s[i + k] & 0x3F);
        res.push_back(cp);
        i += len;
    }
    return res;
}

struct ParseError {};

struct Parser {
    const std::vector<unsigned>& t;
    size_t i = 0;
    Parser(const std::vector<unsigned>& toks) : t(toks) {}
    long peek() { return i < t.size() ? (long)t[i] : -1; }

    int parse() {
        int n = parseAlt();
        if (i != t.size()) throw ParseError();
        return n;
    }
    int parseAlt() {
        int n = parseCat();
        while (peek() == '|') { i++; n = makeNode('|', 0, n, parseCat()); }
        return n;
    }
    int parseCat() {
        std::vector<int> nodes;
        while (true) {
            long c = peek();
            if (c == -1 || c == '|' || c == ')') break;
            nodes.push_back(parseStar());
        }
        if (nodes.empty()) return makeNode('e');
        int n = nodes[0];
        for (size_t k = 1; k < nodes.size(); k++) n = makeNode('.', 0, n, nodes[k]);
        return n;
    }
    int parseStar() {
        int atom = parseAtom();
        if (peek() == '*') { i++; atom = makeNode('*', 0, atom); }
        return atom;
    }
    int parseAtom() {
        long c = peek();
        if (c == '*') throw ParseError();
        if (c == '(') {
            i++;
            int n = parseAlt();
            if (peek() != ')') throw ParseError();
            i++;
            return n;
        }
        if (c == '\\') {
            i++;
            if (i >= t.size()) throw ParseError();
            return makeNode('c', t[i++]);
        }
        i++;
        return makeNode('c', (unsigned)c);
    }
};

const std::vector<unsigned>* gText;

bool match(int node, int pos, const std::function<bool(int)>& cont) {
    const Node& n = pool[node];
    if (n.type == 'c')
        return pos < (int)gText->size() && (*gText)[pos] == n.cp && cont(pos + 1);
    if (n.type == 'e')
        return cont(pos);
    if (n.type == '.')
        return match(n.a, pos, [&](int e) { return match(n.b, e, cont); });
    if (n.type == '|')
        return match(n.a, pos, cont) || match(n.b, pos, cont);
    return match(n.a, pos, [&](int e) { return e > pos && match(node, e, cont); })
           || cont(pos);
}

int countMatches(int root) {
    int N = (int)gText->size();
    int pos = 0, count = 0;
    bool mustAdvance = false;
    while (pos <= N) {
        int foundStart = -1, foundEnd = -1;
        for (int st = pos; st <= N && foundStart < 0; st++) {
            int end = -1;
            match(root, st, [&](int e) {
                if (mustAdvance && st == pos && e == st) return false;
                end = e;
                return true;
            });
            if (end != -1) { foundStart = st; foundEnd = end; }
        }
        if (foundStart < 0) break;
        count++;
        pos = foundEnd;
        mustAdvance = (foundEnd == foundStart);
    }
    return count;
}

std::string strip(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (unsigned char)s[a] <= ' ') a++;
    while (b > a && (unsigned char)s[b - 1] <= ' ') b--;
    return s.substr(a, b - a);
}

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::ifstream reFile(argv[1]);
    std::ifstream textFile(argv[2]);
    std::ofstream out(argv[3]);

    std::vector<std::string> regexes;
    std::string line;
    while (std::getline(reFile, line)) {
        std::string s = strip(line);
        if (!s.empty()) regexes.push_back(s);
    }

    std::vector<int> roots(regexes.size());
    std::vector<bool> bad(regexes.size(), false);
    for (size_t k = 0; k < regexes.size(); k++) {
        try {
            roots[k] = Parser(decode(regexes[k])).parse();
        } catch (ParseError&) {
            bad[k] = true;
        }
    }

    while (std::getline(textFile, line)) {
        std::string s = strip(line);
        if (s.empty()) continue;
        std::vector<unsigned> chars = decode(s);
        gText = &chars;
        std::string row;
        for (size_t k = 0; k < regexes.size(); k++) {
            if (k) row += " ";
            if (bad[k]) row += "-1";
            else row += std::to_string(countMatches(roots[k]));
        }
        out << row << "\n";
    }
    return 0;
}
