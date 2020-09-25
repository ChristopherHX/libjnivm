#include <jnivm/internal/codegen/parseJNIType.h>
#include <algorithm>
#include <cstring>
#include <regex>

const char *jnivm::ParseJNIType(const char *cur, const char *end, std::string &type) {
	auto last = cur;
	switch (*cur) {
	case 'V':
		type = "void";
		break;
	case 'Z':
		if(!JNIVM_FAKE_JNI_SYNTAX) {
		type = "jboolean";
		} else {
			type = "FakeJni::JBoolean";
		}
		break;
	case 'B':
		if(!JNIVM_FAKE_JNI_SYNTAX) {
		type = "jbyte";
		} else {
			type = "FakeJni::JByte";
		}
		break;
	case 'S':
		if(!JNIVM_FAKE_JNI_SYNTAX) {
		type = "jshort";
		} else {
			type = "FakeJni::JShort";
		}
		break;
	case 'I':
		if(!JNIVM_FAKE_JNI_SYNTAX) {
		type = "jint";
		} else {
			type = "FakeJni::JInt";
		}
		break;
	case 'J':
		if(!JNIVM_FAKE_JNI_SYNTAX) {
		type = "jlong";
		} else {
			type = "FakeJni::JLong";
		}
		break;
	case 'F':
		if(!JNIVM_FAKE_JNI_SYNTAX) {
		type = "jfloat";
		} else {
			type = "FakeJni::JFloat";
		}
		break;
	case 'D':
		if(!JNIVM_FAKE_JNI_SYNTAX) {
		type = "jdouble";
		} else {
			type = "FakeJni::JDouble";
		}
		break;
	case '[':
		cur = ParseJNIType(last + 1, end, type);
		if(!JNIVM_FAKE_JNI_SYNTAX) {
			type = "std::shared_ptr<jnivm::Array<" + type + ">>";
		} else {
			if((cur - (last + 1)) == 1) {
				switch (*(last + 1))
				{
				case 'Z':
					type = "std::shared_ptr<FakeJni::JBooleanArray>";
					break;
				case 'B':
					type = "std::shared_ptr<FakeJni::JByteArray>";
					break;
				case 'S':
					type = "std::shared_ptr<FakeJni::JShortArray>";
					break;
				case 'I':
					type = "std::shared_ptr<FakeJni::JIntArray>";
					break;
				case 'J':
					type = "std::shared_ptr<FakeJni::JLongArray>";
					break;
				case 'F':
					type = "std::shared_ptr<FakeJni::JFloatArray>";
					break;
				case 'D':
					type = "std::shared_ptr<FakeJni::JDoubleArray>";
					break;
				default:
					break;
				}
			} else {
				type = "std::shared_ptr<FakeJni::JArray<" + type + ">>";
			}
		}
		break;
	case 'L':
		auto cend = std::find(cur, end, ';');
		if(JNIVM_FAKE_JNI_SYNTAX && (cend - cur) == 16 && !memcmp(cur, "java/lang/String", 16)) {
			type = "std::shared_ptr<FakeJni::JString>";
		} else if(JNIVM_FAKE_JNI_SYNTAX && (cend - cur) == 16 && !memcmp(cur, "java/lang/Object", 16)) {
			type = "std::shared_ptr<FakeJni::JObject>";
		} else {
			type = "std::shared_ptr<" + std::regex_replace(std::string(cur + 1, cend), std::regex("(/|\\$)"),
																"::") +
						">";
		}
		cur = cend;
		break;
	}
	return cur;
}