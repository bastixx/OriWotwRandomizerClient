#include "pch.h"
#include "../../interceptionMacros.h"
#include "../../dllMain.h"


BINDING(System_Char_array*, System_String__ToCharArray, (System_String_o* this_ptr))//System.String$$ToCharArray
BINDING(int32_t, System_Array__get_Length, (System_Array_o* this_ptr)) //System.Array$$get_Length
BINDING(Il2CppObject*, System_Array__GetValue, (System_Array_o* this_ptr, int64_t index)) //System.Array$$GetValue
void printCSString(System_String_o* str){
    if(str)
    {

        auto chars = (System_Array_o*) System_String__ToCharArray(str);
        if(chars)
        {
            auto size = System_Array__get_Length(chars);
            auto str = new char[size];
            for(int i = 0; i < size; i++)
            {
                auto charStructPointer = System_Array__GetValue(chars, i);
                str[i] = *(char*) (charStructPointer + 0x10);
            }
            log(str);

        }
    }
}

INTERCEPT(MessageBox_o*, MessageControllerB__ShowAbilityMessage, (MessageControllerB_o* this_ptr, MessageProvider_o* messageProvider, int32_t ability), {
    //MessageControllerB$$ShowAbilityMessage
    return 0;
});

INTERCEPT(MessageBox_o*, MessageControllerB__ShowShardMessage, (MessageControllerB_o* this_ptr, MessageProvider_o* messageProvider, UnityEngine_GameObject_o* avatar, uint8_t shardType), {
    //MessageControllerB$$ShowShardMessage
    return 0;
});
INTERCEPT(UnityEngine_GameObject_o*, MessageControllerB__ShowSpiritTreeTextMessage, (MessageControllerB_o* this_ptr, MessageProvider_o* messageProvider, UnityEngine_Vector3_o position), {
    //MessageControllerB$$ShowSpiritTreeTextMessage
    return 0;
});

INTERCEPT(void, SeinPickupProcessor__PerformPickupSequence, (SeinPickupProcessor_o* this_ptr, SeinPickupProcessor_CollectableInfo_o* collectableInfo), {
    //SeinPickupProcessor$$PerformPickupSequence
    //noping this removes all pickup animations
          });

INTERCEPT(bool, MessageControllerB__get_AnyAbilityPickupStoryMessagesVisible, (MessageControllerB_o* this_ptr), {
    //MessageControllerB$$get_AnyAbilityPickupStoryMessagesVisible
    return 0;
          });

bool stringHeaderCached = false;
INTERCEPT(System_String_o*, TranslatedMessageProvider_MessageItem__Message, (TranslatedMessageProvider_MessageItem_o* this_ptr, int32_t language), {
    //TranslatedMessageProvider.MessageItem$$GetDescriptor
        auto result = TranslatedMessageProvider_MessageItem__Message(this_ptr, language);
        if(!stringHeaderCached || (result && isInShopScreen()))
            {
            __int64 newString = CSharpLib->call<__int64>("ShopStringRepl", *(__int64*) result);
            stringHeaderCached = true;
            if(newString)
            {
                *(__int64*) result = newString;
            }
        }
        return (System_String_o*) result;
                    });

STATIC(Game_UI_c*, GameController)

BINDING(MessageBox_o*, MessageControllerB__ShowHintSmallMessage, (MessageControllerB_o* this_ptr, MessageDescriptor_o descriptor, UnityEngine_Vector3_o position, float duration))
BINDING(UnityEngine_Vector3_o, OnScreenPositions__get_TopCenter, ())
//MessageBox_o* lastHint = nullptr;
System_String_o* lastMessage = nullptr;
uint32_t lastHandle = 0;
BINDING(void, MessageBox__HideMessageScreenImmediately, (MessageBox_o* this_ptr, int32_t action))
BINDING(void, MessageBox__HideMessageScreen, (MessageBox_o* this_ptr, int32_t action))


MANUAL_BINDING(0x262170, uint32_t, il2cpp_gc_new_weakref, (Il2CppObject* obj, bool track_resurrection))
MANUAL_BINDING(0x262190, Il2CppObject*, il2cpp_gc_get_target, (uint32_t gchandle))
MANUAL_BINDING(0x2621B0, uint32_t, il2cpp_gchandle_free, (uint32_t gchandle))


extern "C" __declspec(dllexport)
void clearMessageBox(MessageBox_o * messageBox){
    try {
        MessageBox__HideMessageScreenImmediately(messageBox, 0);
    } catch(...) {
        debug("Couldn't clear message box, this is fine");
    }
}

extern "C" __declspec(dllexport)
void clearLastHint(){
    if(lastHandle){
        MessageBox_o* lastBox = (MessageBox_o*) il2cpp_gc_get_target(lastHandle);
        if(lastBox)
        {
            clearMessageBox(lastBox);
        }
        il2cpp_gchandle_free(lastHandle);
    }
}

extern "C" __declspec(dllexport)
bool hintsReady(){
    return stringHeaderCached;
}
extern "C" __declspec(dllexport)
MessageBox_o * displayHint(System_String_o * hint, float duration){
    clearLastHint();

    const auto messageController = GameController->static_fields->MessageController;
    const auto box = MessageControllerB__ShowHintSmallMessage(messageController, MessageDescriptor_o{hint, 0, nullptr, nullptr}, OnScreenPositions__get_TopCenter(), duration);
    lastHandle = il2cpp_gc_new_weakref((Il2CppObject*) box, true);

    lastMessage = hint;
    return box;
}

extern "C" __declspec(dllexport)
System_String_o * getCurrentHint(){
    return lastMessage;
}