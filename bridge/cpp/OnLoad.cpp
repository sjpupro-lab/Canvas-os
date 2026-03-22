#include <fbjni/fbjni.h>

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *) {
  return facebook::jni::initialize(vm, [] {});
}
