#include <jnivm/object.h>
#include <jnivm/extends.h>
namespace jnivm {
    class Weak : public Extends<Object> {
    public:
        std::weak_ptr<Object> wrapped;
        
    };
}