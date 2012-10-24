#ifndef SEQINT_H
#define SEQINT_H

struct SeqInt
{
    SeqInt()
        : current_(0)
    {}

    inline
    int operator()()
    { return current_++; }

private:
    int current_;
};

#endif // SEQINT_H
