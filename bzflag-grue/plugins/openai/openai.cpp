#include "bzfsAPI.h"
#include "plugin_utils.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <thread>

class OpenAI : public bz_Plugin
{
    virtual const char* Name()
	{
		return "OpenAI Experiment";
	}
	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~OpenAI() {};

	virtual void Cleanup(void)
	{
		Flush();
	}
};

BZ_PLUGIN(OpenAI)

void OpenAI::Init (const char*)
{
    Register(bz_eRawChatMessageEvent);
}


std::string exec(const char* cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    
    return result;
}

void broadcastMsg(std::string msg)
{
    while (msg.length() > 80)
    {
        int index = 79;
        while (msg[index] != ' ')
            index--;
        bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, msg.substr(0, index).c_str());
        msg = msg.substr(index + 1);
    }

    bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, msg.c_str());
}

void makeOpenAICall(std::string msg)
{
    bz_debugMessage(0, "Message to server");
    std::string cmd = "python3.8 openai_runner.py \"" + msg + "\"";
    std::string output = exec(cmd.c_str());
    bz_debugMessage(0, "Exec finished");
    broadcastMsg(output);
}

void OpenAI::Event(bz_EventData *eventData)
{
	if (eventData->eventType == bz_eRawChatMessageEvent)
	{
		bz_ChatEventData_V2 *data = (bz_ChatEventData_V2*) eventData;
        bz_debugMessage(0, "Event went off");
		
		if (data->to == BZ_ALLUSERS)
        {
            if (data->message.size() > 10)
            {
                bz_debugMessage(0, "Message to allusers");
                std::string beginning = ((std::string) data->message).substr(0,8);
                std::string msg = ((std::string) data->message).substr(8);

                if (strcmp(beginning.c_str(), "Server, ") == 0)
                {
                    std::thread t1(makeOpenAICall, msg);
                    t1.detach();
                }
            }
        }
	}
}

