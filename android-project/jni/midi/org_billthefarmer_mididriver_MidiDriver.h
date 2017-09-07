/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_billthefarmer_mididriver_MidiDriver */

#ifndef _Included_org_billthefarmer_mididriver_MidiDriver
#define _Included_org_billthefarmer_mididriver_MidiDriver
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_billthefarmer_mididriver_MidiDriver
 * Method:    init
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_billthefarmer_mididriver_MidiDriver_init
  (JNIEnv *, jobject);

/*
 * Class:     org_billthefarmer_mididriver_MidiDriver
 * Method:    config
 * Signature: ()[I
 */
JNIEXPORT jintArray JNICALL Java_org_billthefarmer_mididriver_MidiDriver_config
  (JNIEnv *, jobject);

/*
 * Class:     org_billthefarmer_mididriver_MidiDriver
 * Method:    write
 * Signature: ([B)Z
 */
JNIEXPORT jboolean JNICALL Java_org_billthefarmer_mididriver_MidiDriver_write
  (JNIEnv *, jobject, jbyteArray);

/*
 * Class:     org_billthefarmer_mididriver_MidiDriver
 * Method:    setVolume
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_billthefarmer_mididriver_MidiDriver_setVolume
  (JNIEnv *, jobject, jint);

/*
 * Class:     org_billthefarmer_mididriver_MidiDriver
 * Method:    shutdown
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_billthefarmer_mididriver_MidiDriver_shutdown
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif
