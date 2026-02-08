#pragma pack(push, 1)
struct Foo {
    char arr[32];
};
#pragma pack(pop)
struct Foo foo_instance;
#define my_arr ((int*)foo_instance.arr)
int test(int idx) {
    return my_arr[2 * idx];
}
