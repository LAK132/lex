/*
MIT License

Copyright (c) 2018 Lucas Kleiss (LAK132)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <locale>
#include <memory>

namespace lak
{
    using std::string;
    using std::vector;
    using std::unordered_map;
    using std::shared_ptr;
    using std::make_shared;

    template<typename T>
    struct suffix_trie_t
    {
        string key;
        vector<T> values;
        unordered_map<char, shared_ptr<suffix_trie_t<T>>> children;

        suffix_trie_t() {}
        suffix_trie_t(const vector<T> &val) : values(val) {}
        suffix_trie_t(vector<T> &&val) : values(val) {}
        suffix_trie_t(const string str) : key(str) {}
        suffix_trie_t(const string str, const vector<T> &val) : key(str), values(val) {}
        suffix_trie_t(const string str, vector<T> &&val) : key(str), values(val) {}

        inline shared_ptr<suffix_trie_t<T>> find(const string str) const
        {
            if (auto &&it = children.find(str[0]); it != children.end())
                return it->second;
            return nullptr;
        }

        inline shared_ptr<suffix_trie_t<T>> operator[](const string str) const
        {
            return find(str);
        }

        inline bool isTerminal() { return !children.size(); }

        void set(const string str, vector<T> &&val) { set(str, val); }
        void set(const string str, const vector<T> &val)
        {
            if (auto &it = children.find(str[0]); it != children.end() && it->second != nullptr)
            {
                string &k = it->second->key;
                if (str.size() >= k.size() && !std::strncmp(str.c_str(), k.c_str(), k.size()))
                {
                    if (str.size() == k.size())
                    {
                        // they are the same
                        it->second->values = val;
                    }
                    else
                    {
                        // k is the suffix of str
                        it->second->set(str.substr(k.size()), val);
                    }
                }
                else
                {
                    // key slot is taken, but this str doesn't match current slot key.
                    // must split slot into largest common substring
                    size_t same = 0;
                    for (; same < str.size() && same < k.size() && str[same] == k[same]; ++same);

                    string commonstr = str.substr(0, same); // part of the string that's the same
                    string oldstr = k.substr(same);         // rest of the old key string
                    shared_ptr<suffix_trie_t<T>> replacement;

                    if (same == str.size())
                    {
                        // str was the suffix of k
                        // therefore the replacement node is also the node to set
                        replacement = make_shared<suffix_trie_t<T>>(commonstr, val);
                    }
                    else
                    {
                        // str and k have different endings
                        // therefore we must add the new node and the replacement node seperately
                        string setstr = str.substr(same);       // rest of the set key string
                        replacement = make_shared<suffix_trie_t<T>>(commonstr);
                        shared_ptr<suffix_trie_t<T>> set = make_shared<suffix_trie_t<T>>(setstr, val);
                        replacement->children[setstr[0]] = set;
                    }

                    shared_ptr<suffix_trie_t<T>> old = it->second;
                    old->key = oldstr;
                    replacement->children[oldstr[0]] = old;

                    children[commonstr[0]] = replacement;
                }
            }
            else
            {
                children[str] = make_shared<suffix_trie_t<T>>(val);
            }
        }
    };
}

namespace lex
{
    using std::string;
    using std::istream;
    using std::vector;
    using std::unordered_map;
    using std::isspace;

    static std::locale loc;
    enum token_type { END, USER, KEYWORD, SYMBOL };
    static lak::suffix_trie_t<token_type> tokens;
    struct token_t { token_type type; string value; };

    static inline bool is_letter(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    static inline bool is_number(char c)
    {
        return c >= '0' && c <= '9';
    }

    static inline bool is_alphanumeric(char c)
    {
        return is_letter(c) || is_number(c);
    }

    static inline bool is_symbol(char c)
    {
        return !(is_letter(c) || is_number(c) || isspace(static_cast<unsigned char>(c), loc));
    }

    static inline bool hit_word_boundry(char c1, char c2)
    {
        if (c1 == 0) return false;
        if (isspace(static_cast<unsigned char>(c2), loc)) return true;
        if (is_alphanumeric(c1) != is_alphanumeric(c2)) return true;
        return false;
    }

    static token_t next_token(istream &strm)
    {
        string str = "";
        token_t rtn = { END, "" };
        while (strm.good() && isspace(static_cast<unsigned char>(strm.peek()), loc)) strm.get(); // skip whitespace
        for (char p = 0, c = strm.get(); strm.good(); c = strm.get())
        {
            str += c;
            if (hit_word_boundry(p, c))
            {
                // reached the end of the token
                if (auto &&it = tokens.find(str); it != nullptr)
                {
                    // we found a token!
                    rtn.type = it->values[0];
                    rtn.value = str;
                    break;
                }
                else
                {
                    // it wasn't a valid token
                    rtn.type = token_type::USER;
                    rtn.value = str;
                    break;
                }
            }
            p = c;
        }
        if (strm.good())
            strm.unget();
        return rtn;
    }
}

using std::ifstream;
using std::cout;
using std::cin;
using std::string;

int main()
{
    string filename;
    cin >> filename;
    if (ifstream strm(filename, ifstream::in | ifstream::binary); strm.is_open())
    {
        for (lex::token_t t = lex::next_token(strm); t.type != lex::token_type::END; t = lex::next_token(strm))
        {
            cout << t.value << '\n';
        }
    }
    return 0;
}