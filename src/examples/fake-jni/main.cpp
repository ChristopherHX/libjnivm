#include <fake-jni/fake-jni.h>
#include <iostream>

using namespace FakeJni;

class SampleClass : public JObject {
public:
    DEFINE_CLASS_NAME("com/example/SampleClass")

    SampleClass() {
        booleanfield = true;
    }

    JBoolean booleanfield;
    JByte bytefield;
    JShort shortfield;
    JInt intfield;
    JLong longfield;
    JFloat floatfield;
    JDouble doublefield;

    std::shared_ptr<JByteArray> bytearrayfield;
    std::shared_ptr<JShortArray> shortarrayfield;
    std::shared_ptr<JIntArray> intarrayfield;
    std::shared_ptr<JLongArray> longarrayfield;
    std::shared_ptr<JFloatArray> floatarrayfield;
    std::shared_ptr<JDoubleArray> doublearrayfield;

    JDouble JustAMemberFunction(std::shared_ptr<JIntArray> array) {
        for (size_t i = 0; i < array->getSize(); i++) {
            std::cout << "Value of (*array)[" << i << "] = " << (*array)[i] << "\n"; 
        }
        return 3.6;
    }

    inline static void exampleStaticFunction(JDouble d) {
        std::cout << "From ExampleClass: " << d << std::endl;
    }
};

BEGIN_NATIVE_DESCRIPTOR(SampleClass)
{ Field<&SampleClass::booleanfield>{}, "booleanfield" },
{ Field<&SampleClass::bytefield>{}, "bytefield" },
{ Field<&SampleClass::shortfield>{}, "shortfield" },
{ Field<&SampleClass::intfield>{}, "intfield" },
{ Field<&SampleClass::longfield>{}, "longfield" },
{ Field<&SampleClass::floatfield>{}, "floatfield" },
{ Field<&SampleClass::doublefield>{}, "doublefield" },
{ Field<&SampleClass::bytearrayfield>{}, "bytearrayfield" },
{ Field<&SampleClass::shortarrayfield>{}, "shortarrayfield" },
{ Field<&SampleClass::intarrayfield>{}, "intarrayfield" },
{ Field<&SampleClass::longarrayfield>{}, "longarrayfield" },
{ Function<&SampleClass::JustAMemberFunction>{}, "JustAMemberFunction" },
{ Function<&exampleStaticFunction>{}, "exampleStaticMemberFunction" },
{ Function<&exampleStaticFunction>{}, "exampleStaticFunction", JMethodID::STATIC_FUNC },
END_NATIVE_DESCRIPTOR

int main(int argc, char** argv) {
    Jvm jvm;
    jvm.registerClass<SampleClass>();
    auto SampleClass_ = jvm.findClass("SampleClass");
    LocalFrame frame(jvm);
    jobject ref = frame.getJniEnv().createLocalReference(std::make_shared<SampleClass>());
    jfieldID fieldid = frame.getJniEnv().GetFieldID(frame.getJniEnv().GetObjectClass(ref), "booleanfield", "Z");
    jboolean value = frame.getJniEnv().GetBooleanField(ref, fieldid);
    std::cout << "booleanfield has value " << (bool)value << "\n";
    std::shared_ptr<SampleClass> refAsObj = std::dynamic_pointer_cast<SampleClass>(frame.getJniEnv().resolveReference(ref));
    refAsObj->booleanfield = false;
    value = frame.getJniEnv().GetBooleanField(ref, fieldid);
    std::cout << "booleanfield has changed it's value to " << (bool)value << "\n";

    jmethodID method = frame.getJniEnv().GetMethodID(frame.getJniEnv().GetObjectClass(ref), "JustAMemberFunction", "([I)D");
    auto a = frame.getJniEnv().NewIntArray(23);
    jint* carray = frame.getJniEnv().GetIntArrayElements(a, nullptr);
    for (size_t i = 0; i < frame.getJniEnv().GetArrayLength(a); i++) {
        carray[i] = 1 << i;
    }
    frame.getJniEnv().ReleaseIntArrayElements(a, carray, 0);
    jdouble ret = frame.getJniEnv().CallDoubleMethod(ref, method, a);
    std::cout << "JustAMemberFunction returned " << ret << "\n";

    jmethodID exampleStaticMemberFunction = frame.getJniEnv().GetMethodID(frame.getJniEnv().GetObjectClass(ref), "exampleStaticMemberFunction", "(D)V");
    frame.getJniEnv().CallVoidMethod(ref, exampleStaticMemberFunction, 7.9);

    jmethodID exampleStaticFunction = frame.getJniEnv().GetMethodID(frame.getJniEnv().GetObjectClass(ref), "exampleStaticMemberFunction", "(D)V");
    frame.getJniEnv().CallStaticVoidMethod(frame.getJniEnv().GetObjectClass(ref), exampleStaticFunction, 3.8);
    return 0;
}