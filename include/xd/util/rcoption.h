#ifndef __XD_UTIL_RCOPTION_H__
#define __XD_UTIL_RCOPTION_H__

#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <cassert>
#include <stdexcept>
#include <new>
#include <map>

#include <xd/topdef.h>
#include <xd/util/memory.h>

namespace xd { namespace util {

namespace {

static inline
int gen_optval(void) {
    static int option_val = 256;
    return ++option_val;
}

static inline
std::string comment(const std::string& lines, char commenttag = '#') {
    std::string str;
    char ch;
    size_t len = lines.size();
    str.push_back(commenttag);
    for (size_t i = 0; i < len; ++i) {
        ch = str[i];
        str.push_back(ch);
        if (ch == '\n') {
            str.push_back(commenttag);
        }
    }
    return str;
}

static inline
bool tokenize_option_line(const std::string& line,
                          std::string* key, std::string* value,
                          char commenttag = '#') {
    assert(key);
    assert(value);

    const std::string::size_type length = line.size();
    std::string::size_type pos = 0;

    while (pos < length && isspace(line[pos])) pos++;

    if (pos == length || line[pos] == commenttag) return false;

    key->clear();
    value->clear();

    while (pos < length && !isspace(line[pos]) && line[pos] != '=') {
        key->push_back(line[pos]);
        pos++;
    }

    while (pos < length && line[pos] != '=') pos++;
    if (pos < length) pos++;

    while (pos < length && isspace(line[pos])) pos++;

    while (pos < length && !isspace(line[pos])) {
        value->push_back(line[pos]);
        pos++;
    }

    return true;
}

} // namespace

class rcoption {
    friend class rcoptions;

  public:
    typedef enum {
        NO_ARGUMENT = no_argument,
        REQUIRED_ARGUMENT = required_argument,
        OPTIONAL_ARGUMENT = optional_argument,
    } argument_type;
    typedef enum {
        DEFAULT = 0,
        RCFILE = 1,
        COMMAND_LINE = 2,
        HOT_PLUGING = 4,
    } setter_type;

  public:
    argument_type get_argument_type(void) const {
        return m_arg_type;
    }
    int short_option(void) const {
        return m_short_option;
    }
    int optval(void) const {
        return m_optval;
    }
    bool switchoned(setter_type* setter = 0) const {
        if (setter != 0) {
            *setter = m_setter;
        }
        return m_switch;
    }
    std::string argument(setter_type* setter = 0) const {
        if (setter != 0) {
            *setter = m_setter;
        }
        return m_argument;
    }
    setter_type setter(void) const {
        return m_setter;
    }
    int setters(void) const {
        return m_setters;
    }

  public:
    rcoption(const std::string long_option,
             int short_option,
             argument_type arg_type,
             const std::string& usage,
             const std::string& argument,
             int setters):
      m_long_option(long_option),
      m_short_option(short_option),
      m_optval(short_option),
      m_arg_type(arg_type),
      m_argument(argument),
      m_setters(setters),
      m_setter(DEFAULT),
      m_switch(false) {
        if (m_long_option.empty()) {
            throw std::invalid_argument(_("empty long option"));
        }
        if (!isalnum(m_short_option)) {
            throw std::invalid_argument(_("unexpected short option") +
                                        std::string(" -- ") + std::string(1, m_short_option));
        }
        m_usage_portion = gen_usage_portion(usage);
    }
    rcoption(int short_option,
             argument_type arg_type,
             const std::string& usage,
             const std::string& argument,
             int setters):
      m_short_option(short_option),
      m_optval(0),
      m_arg_type(arg_type),
      m_argument(argument),
      m_setters(setters),
      m_setter(DEFAULT),
      m_switch(false) {
        // assert((0 == (m_setters & RCFILE)) || !m_name.empty());
        if (!isalnum(m_short_option)) {
            throw std::invalid_argument(_("unexpected short option") +
                                        std::string(" -- ") + std::string(1, m_short_option));
        }
        m_usage_portion = gen_usage_portion(usage);
    }
    rcoption(const std::string& long_option,
             argument_type arg_type,
             const std::string& usage,
             const std::string& argument,
             int setters):
      m_long_option(long_option),
      m_short_option(0),
      m_optval(gen_optval()),
      m_arg_type(arg_type),
      m_argument(argument),
      m_setters(setters),
      m_setter(DEFAULT),
      m_switch(false) {
        // assert((0 == (m_setters & RCFILE)) || !m_name.empty());
        if (m_long_option.empty()) {
            throw std::invalid_argument(_("empty long option"));
        }
        m_usage_portion = gen_usage_portion(usage);
    }

  private:
    bool switchon(setter_type setter) {
        if ((setter & m_setters) &&
            (setter >= m_setter)) {
            m_switch = true;
            m_setter = setter;
            return true;
        }
        return false;
    }
    bool set_argument(const std::string& argument, setter_type setter) {
        if ((setter & m_setters) &&
            (setter >= m_setter)) {
            m_argument = argument;
            m_setter = setter;
            return true;
        }
        return false;
    }
    std::string usage_portion(void) const {
        return m_usage_portion;
    }
    std::string optstring_portion(void) const {
        std::string str;
        if (m_short_option != '\0') {
            str.push_back(m_short_option);
            if (m_arg_type == REQUIRED_ARGUMENT) {
                str += ":";
            }
            else if (m_arg_type == OPTIONAL_ARGUMENT) {
                str += "::";
            }
        }
        return str;
    }
    bool can_get_rcoption(void) const {
        return !m_long_option.empty();
    }
    struct option option(void) const {
        assert(!m_long_option.empty());
        struct option option;
        option.name = m_long_option.c_str();
        option.has_arg = m_arg_type;
        option.flag = 0;
        if (m_short_option != 0) {
            option.val = m_short_option;
        }
        else {
            option.val = m_optval;
        }
        return option;
    }
    std::string dump(const std::string& name) const {
        if (m_setters & RCFILE) {
            return name + " = " + m_argument;
        }
        return "";
    }
    std::string dump_with_comment(const std::string& name, char commenttag = '#') const {
        if (m_setters & RCFILE) {
            return comment(m_usage_portion, commenttag) + "\n" + dump(name);
        }
        return "";
    }
    std::string gen_usage_portion(const std::string& usage) const {
        std::ostringstream sout;
        sout << "\t";
        if (m_short_option != 0) {
            sout << "-" << static_cast<char>(m_short_option);
        }
        if (m_short_option != 0 && !m_long_option.empty()) {
            sout << ", ";
        }
        if (!m_long_option.empty()) {
            sout << "--" << m_long_option;
            if (m_arg_type != NO_ARGUMENT) {
                sout << "=";
            }
        }
        switch (m_arg_type) {
            case REQUIRED_ARGUMENT: {
                sout << "REQUIRED_ARGUMENT";
                break;
            }
            case OPTIONAL_ARGUMENT: {
                sout << "OPTIONAL_ARGUMENT";
                break;
            }
            default: {
                break;
            }
        }
        sout << "\n";
        sout << "\t\t" << usage << "\n";
        if (m_arg_type == REQUIRED_ARGUMENT || m_arg_type == OPTIONAL_ARGUMENT) {
            sout << "\t\t" << "default as \"" << m_argument << "\"" << "\n";
        }
        return sout.str();
    }

  private:
    std::string m_long_option;
    int m_short_option;
    int m_optval;
    argument_type m_arg_type;
    setter_type m_setter;
    int m_setters;
    std::string m_usage_portion;
    std::string m_argument;
    bool m_switch;
};
    
class rcoptions {
  public:
    void add(const std::string& name, const rcoption& option) {
        std::pair<std::map<std::string, rcoption>::iterator, bool> ret =
                m_options.insert(std::make_pair(name, option));
        if (!ret.second) {
            throw std::logic_error(_("invalid duplicate option attempt"));
        }
        return;
    }
    size_t size(void) const {
        return m_options.size();
    }
    size_t count(const std::string& name) const {
        return m_options.count(name);
    }
    const rcoption& find(const std::string& name) const {
        std::map<std::string, rcoption>::const_iterator it = m_options.find(name);
        if (it == m_options.end()) {
            throw std::logic_error(_("cannot locate the option") + std::string(" -- ") + name);
        }
        return it->second;
    }
    rcoption& find(const std::string& name) {
        std::map<std::string, rcoption>::iterator it = m_options.find(name);
        if (it == m_options.end()) {
            throw std::logic_error(_("cannot locate the option") + std::string(" -- ") + name);
        }
        return it->second;
    }
    void dump(std::ostream& out, const std::string& head, char commenttag = '#') {
        assert(out.good());
        out << comment(head, commenttag) << std::endl;
        for (std::map<std::string, rcoption>::const_iterator it = m_options.begin();
             it != m_options.end();
             ++it) {
            out << it->second.dump_with_comment(it->first, commenttag) << std::endl;
        }
        return;
    }
    std::string usage(const std::string& head) const {
        std::map<std::string, rcoption>::const_iterator it;
        std::string u = head + "\n\n";
        for (it = m_options.begin(); it != m_options.end(); ++it) {
            u += it->second.usage_portion();
            u += "\n";
        }
        return u;
    }
    void load(std::istream& in, rcoption::setter_type setter = rcoption::RCFILE, char commenttag = '#') {
        assert(in.good());
        assert(setter >= rcoption::RCFILE);

        std::string line;
        int lineno;
        std::string key;
        std::string value;
        bool tokenized;
        std::map<std::string, rcoption>::iterator it;

        lineno = 0;
    
        while (in && getline(in, line, '\n')) {
            lineno++;
            tokenized = tokenize_option_line(line, &key, &value);
            if (!tokenized) {
                continue;
            }
            if (key.empty() || value.empty()) {
                std::ostringstream sout;
                sout << _("Bad file option in line") << " " << lineno;
                throw std::invalid_argument(sout.str());
            }
            it = m_options.find(key);
            if (it == m_options.end()) {
                throw std::invalid_argument(_("Unknown option") + std::string(" ") + key);
            }
            if (it->second.get_argument_type() == rcoption::NO_ARGUMENT) {
                throw std::invalid_argument(_("Should not set a argument for a non-argument option") +
                                            std::string(" ") +
                                            key);
            }
            (void)it->second.set_argument(optarg ? optarg : "", setter);
        }

        return;
    }
    void load(int argc, char* argv[], rcoption::setter_type setter = rcoption::COMMAND_LINE) {
        int c;
        int option_index = 0;
        const std::string optstring = "+" + optstr();
        xd::util::scoped_array<struct option> long_options(newrcoptions());
        std::map<std::string, rcoption>::iterator it;

        optind = 0;
        opterr = 1;
        for (;;) {
            c = getopt_long(argc, argv, optstring.c_str(), long_options.get(), &option_index);
            if (c == -1) break;
            if (c == '?' || c == ':') {
                throw std::invalid_argument(_("invalid option or insufficient argument"));
            }
            
            for (it = m_options.begin(); it != m_options.end(); ++it) {
                if (c != it->second.short_option() && c != it->second.optval()) continue;

                if (it->second.get_argument_type() == rcoption::NO_ARGUMENT) {
                    (void)it->second.switchon(setter);
                }
                else if (it->second.get_argument_type() == rcoption::REQUIRED_ARGUMENT) {
                    (void)it->second.set_argument(optarg ? optarg : "", setter);
                }
                else {
                    assert(it->second.get_argument_type() == rcoption::OPTIONAL_ARGUMENT);
                    (void)it->second.switchon(setter);
                    (void)it->second.set_argument(optarg ? optarg : "", setter);
                }
                break;
            }
            if (it == m_options.end()) {
                throw std::invalid_argument(_("unknown option"));
            }
        }
        if (optind < argc) {
            std::ostringstream os;
            os << _("non-option ARGV-elements: ");
            while (optind < argc) {
                os << argv[optind++] << " ";
            }
            os << std::endl;
            throw std::invalid_argument(os.str());
        }

        return;
    }

  private:
    std::string optstr(void) const {
        std::string optstr = ":";
        for (std::map<std::string, rcoption>::const_iterator it = m_options.begin();
             it != m_options.end();
             ++it) {
            optstr += it->second.optstring_portion();
        }
        return optstr;
    }
    struct option* newrcoptions(void) const {
        size_t n = m_options.size();
        size_t r = 0;
        std::map<std::string, rcoption>::const_iterator it;
        struct option* options = 0;

        try {
            options = new struct option[n + 1];
            if (options == 0) {
                throw std::runtime_error(_("bad allocation") +
                                         std::string(" -- ") +
                                         _("new struct option"));
            }
            for (it = m_options.begin(); it != m_options.end(); ++it) {
                if (!it->second.can_get_rcoption()) continue;
                options[r++] = it->second.option();
            }
            options[r].name = NULL;
            options[r].has_arg = 0;
            options[r].flag = 0;
            options[r].val = 0;
            r++;
        }
        catch (...) {
            // in case of those signal throwable.
            delete[] options;
            throw;
        }
        return options;
    }

  private:
    std::map<std::string, rcoption> m_options;
};

} // namespace util
} // namespace xd

#endif
