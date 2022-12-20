#include <random>
#include <ctime>
#include <string>
#include <fstream>
#include <iostream>


using State = int;

//interface class for a state
class IState {
public:
    virtual bool contains(State) const = 0;
    virtual ~IState() = default;
};

//discrete state
class DiscreteState: public IState {
private:
    State s0;
public:
    DiscreteState(State s0): s0(s0) {}
    DiscreteState& operator=(const DiscreteState &right) {
        s0 = right.s0;
        return *this;
    }
    bool contains(State s) const override {
        return s == s0;
    }
};

//segmented state
class SegmentState: public IState {
private:
    State begin_s0, end_s0;
public:
    SegmentState(State begin_s0, State end_s0) :
        begin_s0(begin_s0), end_s0(end_s0) {}
    SegmentState& operator=(const SegmentState &right) {
        begin_s0 = right.begin_s0;
        end_s0 = right.end_s0;
        return *this;
    }
    bool contains(State s) const override {
        return begin_s0 <= s && end_s0 >= s;
    }
};

/* Classes for representing complex states. In fact, decorator pattern has been used, though it may be unoptimal.
 * They aren't intended to be created on stack, since it is hard to correctly work with temporary instances of these
 * classes without shared pointers. Therefore, they only have private constructors. */

//Inversed state
class InverseState: public IState {
private:
    const IState* base;
    InverseState(const IState* base) : base(base) {}
    InverseState() : base(nullptr) {}
public:
    ~InverseState() {
        delete base;
    }
    inline static InverseState* create() {
        return new InverseState();
    }
    inline static InverseState* create(const IState* base) {
        return new InverseState(base);
    }
    bool contains(State s) const override {
        return !base->contains(s);
    }

};

//An intersection of two states
class IntersectState: public IState {
private:
    const IState *first, *second;
    IntersectState(const IState* first, const IState* second) : first(first), second(second) {}
    IntersectState() : first(nullptr), second(nullptr) {}
public:
    ~IntersectState() {
        delete first;
        delete second;
    }
    inline static IntersectState* create() {
        return new IntersectState();
    }
    inline static IntersectState* create(const IState* first, const IState* second) {
        return new IntersectState(first, second);
    }
    bool contains(State s) const override {
        return first->contains(s) && second->contains(s);
    }
};

//A unification of two states
class UnifyState: public IState {
private:
    const IState &first, &second;
public:
    UnifyState(const IState &first, const IState &second) : first(first), second(second) {}
    UnifyState& operator=(const UnifyState& right)
    bool contains(State s) const {
        return first.contains(s) || second.contains(s);
    }
};

class ProbabilityTest {
private:
    State const E_min, E_max;
public:
    ProbabilityTest(State E_min, State E_max):
        E_min(E_min), E_max(E_max) {}
    float test (
            IState &system,
            unsigned test_count,
            unsigned seed) const {
        std::default_random_engine reng(seed);
        std::uniform_int_distribution<int> dstr(E_min, E_max);
        dstr(reng);
        unsigned good = 0;
        for (unsigned cnt = 0; cnt != test_count; cnt++)
            if (system.contains(dstr(reng)))
                good++;
        return static_cast<float>(good) / static_cast<float>(test_count);
    }
};

int main() {
    const int base = 10; //base of power
    const int ncall = 1000; //number of calls for every test
    const int bound = 1000; //boundaries of random energies
    std::string ftype("_ordered.out");
    ProbabilityTest prob(-bound, bound);

    //test a segmented state
    SegmentState ordered(0, bound / 2);
    int n = 1;
    for (int pow = 0; pow <= 6; pow++) {
        std::ofstream fout(std::to_string(pow) + ftype);
        for (int i = 0; i < ncall; i++)
           fout << prob.test(ordered, n, std::clock()) << std::endl;
        fout.close();
        n *= base;
    }
    ftype = "_random.out";
    n = 1;

    //test a randomized state. This isn't uniform distribution, but still is random.
    int pos = -bound;
    std::default_random_engine randeng(std::clock());
    std::uniform_int_distribution<int> distr(1, 4);
    auto random = UnifyState(DiscreteState(pos), DiscreteState(pos += distr(randeng)));
    for (int i = 2; i < bound / 2; i++)
        random = UnifyState(random,  DiscreteState(pos += distr(randeng))); //basically a unification of bound/2 DiscreteStates

    //kinda slow due to a lot of object calls.
    for (int pow = 0; pow <= 6; pow++) {
        std::ofstream fout(std::to_string(pow) + ftype);
        for (int i = 0; i < ncall; i++)
            fout << prob.test(random, n, std::clock()) << std::endl;
        fout.close();
        n *= base;
    }
    delete random;
    return 0;
}
