#include <iostream>
#include <utility>
#include <cstring>
#include <memory>

using namespace std;


//#define FOOBASECOUT
//#define STACKAT
class FooBase {
public:
	FooBase(int id) { SetString(id); }
	FooBase(const FooBase& foobase) : objectid_(foobase.objectid_) {
#ifdef FOOBASECOUT
		cout << "FooBase(" << objectid_ << "): copy ctor";
#endif
	}
	FooBase(FooBase &&foobase) noexcept : objectid_(std::move(foobase.objectid_)) {
#ifdef FOOBASECOUT
		cout << "FooBase(" << objectid_ << "): move ctor";
#endif
	}
	FooBase& operator=(const FooBase& rhs) {
		objectid_= rhs.objectid_;
#ifdef FOOBASECOUT
		cout << "FooBase(" << objectid_ << "): copy assign op";
#endif
		return *this;
	}
	FooBase& operator=(FooBase&& rhs) noexcept {
		//not using objectid_ = rhs.objectid_ so that we can follow the ids
		using std::swap;
		swap(objectid_, rhs.objectid_);
#ifdef FOOBASECOUT
		cout << "FooBase(" << objectid_ << "): copy move op";
#endif
		return *this;
	}
	virtual ~FooBase() {};
	void swap(FooBase &rhs) {
		using std::swap;
		swap(objectid_, rhs.objectid_);
#ifdef FOOBASECOUT
		cout << "FooBase(" << objectid_ << " <--> " << rhs.objectid_ << "): swap (post swap values)";
#endif
	}
	string ObjectID() const { return objectid_; };

private:
	string objectid_;
	void SetString(int id) {
		objectid_ = static_cast<char>(id + 'A');
	}
};

class Foo : public FooBase {
private:
	constexpr static const char* default_fill_ = "Insight";
	//counterid exists to the object.
	static int idcounter;
	//const int also means there are no implicit move operations.
	const int id;
	string name_;
	size_t buffersize_;
	char* pbuffer_;

public:
	Foo(string name, size_t buffersize = 0, char* pbuffer = nullptr)
	: FooBase(idcounter), id(idcounter), name_(name), buffersize_(buffersize), pbuffer_(pbuffer)
	{
		if(!pbuffer_) {
			buffersize_ = sizeof(default_fill_);
			pbuffer_ = new char[buffersize_];
			memcpy(pbuffer_, default_fill_, buffersize_);
		}
		++idcounter;
	}
	~Foo() {
		delete[] pbuffer_;
	}
	//can't pass by value, ambiguous with the move ctor
	Foo(const Foo& foo)
	: FooBase(foo), id(idcounter++), name_(foo.name_), buffersize_(foo.buffersize_) {
		pbuffer_ = new char[buffersize_];
		memcpy(pbuffer_, foo.pbuffer_, buffersize_);
		cout << "Foo(" << id << "): copy ctor" << endl;
	}
	//Note noexcept!
	Foo(Foo&& rhs) noexcept
	: FooBase(std::move(rhs)), id(idcounter++), name_(std::move(rhs.name_)), buffersize_(rhs.buffersize_), pbuffer_(rhs.pbuffer_)  {
		//Make it safe to delete rhs
		rhs.pbuffer_ = nullptr;
		cout << "Foo(" << id << "): move ctor" << endl;
	}
	//can't pass by value, ambiguous with the move assignment operator
	Foo& operator=(const Foo &rhs) {
		if(this == &rhs) return *this;
		//Not the most efficient copy assignment, but this is a move example class
		//Also, this how to get a strong guarantee if FooBase assignment operator can throw
		Foo tmp(rhs);
		swap(tmp);
		cout << "Foo(" << id << "): copy assignment operator" << endl;
		return *this;
	}
	//Note noexcept!
	Foo& operator=(Foo &&rhs) noexcept {
		if(this == &rhs) return *this;
		FooBase::operator=(std::move(rhs));
		//Release our own resources 1st:
		name_.clear();
		name_.shrink_to_fit();
		delete pbuffer_;
		name_ = std::move(rhs.name_);
		buffersize_ = rhs.buffersize_;
		pbuffer_ = rhs.pbuffer_;
		rhs.pbuffer_ = nullptr;
		cout << "Foo(" << id << "): move assignment operator" << endl;
		return *this;
	}
	void swap(Foo &rhs) noexcept {
		using std::swap;
		FooBase::swap(rhs);
		swap(name_, rhs.name_);
		swap(buffersize_, rhs.buffersize_);
		swap(pbuffer_, rhs.pbuffer_);
		cout << "Foo(" << id << "): swap called" << endl;
	}
	friend void swap(Foo &lhs, Foo &rhs) noexcept {
		lhs.swap(rhs);
		cout << "Foo friend: swap called" << endl;
	}
	string name() const { //returns RVALUE - prvalue
		return name_;
	}
	//note how we need 2 functions for setname
	void setname(const string& name) {
		name_ = name;
		cout << "Foo:setname: Copy assign" << endl;
	}
	//std::string move assign is no except so this is too
	void setname(string&& name) noexcept {
		name_ = std::move(name);
		cout << "Foo:setname: Move assign" << endl;
	}
	//We only need one function for setname2 for the same results
	template<typename T>
	void setname2(T&& name) {
		name_ = std::forward<T>(name);
	}
	template<typename T>
	void setname3(T&& name) {
		static_assert(!std::is_pointer<T>::value, "Pointers only");
		name_ = std::forward<T>(name);
	}
	int buffersize() const {
		return buffersize_;
	}
	char& operator[](int i) {  //RETURNS lvalue, note reference
		return name_[i];
	}
};
int Foo::idcounter = 0;

Foo FooFactory(const int whatfoo) {
	//With move we can return by value cheaply even if elision copy isn't possible
	return Foo("move me out");
}

void __attribute__((noinline)) FuncValue(Foo foo) {
	cout << "Value Address: " << &foo << endl;
#ifdef STACKAT
	cout << "Size: " << sizeof(foo) << endl;
	int a;
	cout << "Stack at: " << &a << endl;
#endif
}

void __attribute__((noinline)) FuncCLRef(const Foo &foo) {
	cout << "CLRef Address: " << &foo << endl;
#ifdef STACKAT
	cout << "Size: " << sizeof(foo) << endl;
	int a;
	cout << "Stack at: " << &a << endl;
#endif

}

//__attribute__((noinline)) just makes sure the function isn't inlined
//These functions cout either REF or *R*REF
void __attribute__((noinline)) FuncRef(Foo& foo) {
	cout << "Ref Address: " << &foo << endl;
}

void __attribute__((noinline)) FuncRef(Foo&& foo) {
	cout << "RRef Address: " << &foo << endl;
}

//This function will fail if cout is uncommented
//Also, template<typename T> void FuncRef(T&& t) won't work, endl can't resolve.
template<typename T>
void FuncFail(T&& foo) {
	FuncRef(std::forward<T>(foo));
	//cout << &foo << endl;  //invalid overload of endl
}

Foo ValFuncVal(Foo foo) {
	FuncRef(foo);
	return foo;
}

Foo MakeFoo() {
	return Foo("MakeFoo");
}

std::string MakeString() {
	return std::string("MakeString");
}

int main() {
	cout << "Function Moves:" << endl;
	Foo copyme("five");
	Foo thecopy("back");
	Foo& theref = thecopy;
	FuncFail(thecopy);
	cout << "Testing move assign op" << endl;
	thecopy = move(theref);
	thecopy = MakeFoo();
	Foo myfoo(ValFuncVal(MakeFoo()));
	FuncRef(myfoo);
	cout << "Testing copy ctor" << endl;
	Foo foo("John");
	Foo foo2(foo);

	const std::string Sam("Sam");
	foo2.setname2(Sam);
	foo2.setname2("Jerry");
	foo2.setname2(MakeString());

	cout << "Testing copy assign op" << endl;
	foo = foo2;
	foo2 = foo;

	cout << "foo";
	FuncRef(foo);
	cout << "foo2";
	FuncRef(foo2);
	cout << "Perfect Forwarding" << endl;
	FuncFail(foo);
	FuncFail(std::move(foo));

	cout << "RVO0" << endl;
	FuncValue(foo2);
	FuncValue(foo2);
	cout << "RVO1" << endl;
	FuncValue(Foo("first"));
	cout << "RVO2" << endl;
	Foo foom(MakeFoo());
	FuncValue(foom);
	cout << "RVO3" << endl;
	FuncValue(MakeFoo());
	cout << "RVO4" << endl;
	Foo rfoo = ValFuncVal(foo);
	FuncRef(rfoo);
	cout << "RVO5" << endl;

	Foo& reffoo = foo2;
	FuncValue(reffoo);
	Foo foo3(foo);
	FuncRef(foo3);

	return 0;
}
