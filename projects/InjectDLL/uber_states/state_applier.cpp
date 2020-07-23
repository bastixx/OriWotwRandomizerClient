#include <macros.h>
#include <csharp_bridge.h>
#include <uber_states/state_applier.h>

#include <Il2CppModLoader/common.h>
#include <Il2CppModLoader/console.h>
#include <Il2CppModLoader/interception_macros.h>

#include <unordered_map>

using namespace modloader;

namespace uber_states
{
    namespace
    {
        std::unordered_map<int32_t, applier_intercept> applier_intercepts;

        STATIC_IL2CPP_BINDING(Moon, UberStateController, void, ApplyAll, (int32_t context));
        IL2CPP_INTERCEPT(, NewSetupStateController, void, ApplyKnownState, (app::NewSetupStateController* this_ptr, int32_t stateGUID, int32_t context)) {
            auto it = applier_intercepts.find(stateGUID);
            if (it != applier_intercepts.end())
                stateGUID = it->second(this_ptr, stateGUID, context);

            NewSetupStateController::ApplyKnownState(this_ptr, stateGUID, context);
        }

        void intercept_state(std::string const& command, std::vector<console::CommandParam> const& params)
        {
            if (params.size() != 2)
            {
                console::console_send("invalid number of parameters.");
                return;
            }

            if (!params[0].name.empty() || !params[1].name.empty())
            {
                console::console_send("invalid, does not support named parameters.");
                return;
            }

            int first;
            if (!console::try_get_int(params[0], first))
            {
                console::console_send("invalid first parameter, not an integer.");
                return;
            }

            int second;
            if (!console::try_get_int(params[1], second))
            {
                console::console_send("invalid second parameter, not an integer.");
                return;
            }


            register_applier_redirect(first, second);
            UberStateController::ApplyAll(0);
        }

        void add_applier_intercept_commands()
        {
            console::register_command("intercept_state", intercept_state);
        }

        CALL_ON_INIT(add_applier_intercept_commands);
    }

    void register_applier_intercept(int32_t state, applier_intercept callback)
    {
        if (applier_intercepts.find(state) != applier_intercepts.end())
            trace(MessageType::Warning, 1, "init", "registering same applier state twice, overwriting.");

        applier_intercepts[state] = callback;
    }

    void register_applier_intercept(std::vector<int32_t> states, applier_intercept callback)
    {
        for (auto state : states)
            register_applier_intercept(state, callback);
    }

    void register_applier_redirect(int32_t state, int32_t new_state)
    {
        if (applier_intercepts.find(state) != applier_intercepts.end())
            trace(MessageType::Warning, 1, "init", "registering same applier state twice, overwriting.");

        applier_intercepts[state] = [new_state](auto, auto, auto) -> int32_t { return new_state; };
    }

    void register_applier_redirect(std::vector<std::pair<int32_t, int32_t>> states)
    {
        for (auto state : states)
            register_applier_redirect(state.first, state.second);
    }
}
