#ifndef __XD_ICE__
#define __XD_ICE__

module XD {

exception XDException {
    int code;
    string details;
};

sequence<string> StringVector;

interface QueryServer {
    void executeQuery(in string query, out StringVector records) throws XDException;
    void close() throws XDException;
};

};

#endif
