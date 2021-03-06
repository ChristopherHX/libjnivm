#include <fake-jni/fake-jni.h>

using namespace FakeJni;

class SampleClass : public JObject {
public:
    DEFINE_CLASS_NAME("com/example/SampleClass")

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
END_NATIVE_DESCRIPTOR

int main(int argc, char** argv) {
    Jvm jvm;
    jvm.registerClass<SampleClass>();
    auto SampleClass_ = jvm.findClass("SampleClass");
    LocalFrame frame(jvm);
    jobject ref = frame.getJniEnv().createLocalReference(std::make_shared<SampleClass>());
    return 0;
}