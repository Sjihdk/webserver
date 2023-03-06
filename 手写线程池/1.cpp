#include<bits/stdc++.h>

using namespace std;


void adjust(int *a,int index,int size){
    int l=index*2+1;
    int r=index*2+2;
    int m=index;//三个数中最大数下标
    if(l<size&&a[l]>a[m]) m=l;
    if(r<size&&a[r]>a[m]) m=r;
    if(m==index) return ;
    swap(a[m],a[index]);
    adjust(a,m,size);
}

void heapsort(int *a,int size){
    for(int i=(size-2)/2;i>=0;i--){
        adjust(a,i,size);
    }
    for(int i=size-1;i>0;i--){
        swap(a[i],a[0]);
        adjust(a,0,i);//此时size为i，除去了最后一个下标
    }
}

class Base{
public:
    Base() {}
    ~Base() {}
    void print() {
        std::cout << "I'm Base" << endl;
    }////

    virtual void i_am_virtual_foo() {cout<<1;}
};

class Sub: public Base{
public:
    Sub() {}
    ~Sub() {}
    void print() {
        std::cout << "I'm Sub" << endl;
    }

    virtual void i_am_virtual_foo() {cout<<2;}
};
class c{
public:
 void print(){
    cout<<6;
 }
};

int main(){
 
    Base *b=new Base();
    Sub *s=new Sub();

    Sub * p=reinterpret_cast<Sub*>(b);
    p->i_am_virtual_foo();

    return 0;
}