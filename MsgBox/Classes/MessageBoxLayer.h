#pragma once
#include "CommonTitleLayer.h"

enum struct MsgButton : uint32_t
{
	Default = 0x7fffffff,
	OK = 1U,
	Cancel = 2U,
	Close = 4U,
	OKCancel = 3U,
};
inline MsgButton operator & (MsgButton e1, MsgButton e2)
{
	return MsgButton((uint32_t)e1 & (uint32_t)e2);
}
inline MsgButton operator | (MsgButton e1, MsgButton e2)
{
	return MsgButton((uint32_t)e1 | (uint32_t)e2);
}
inline MsgButton operator ^ (MsgButton e1, MsgButton e2)
{
	return MsgButton((uint32_t)e1 ^ (uint32_t)e2);
}
inline MsgButton operator ~ (MsgButton e)
{
	return MsgButton(~(uint32_t)e);
}

//----------------------------------------------------------------------------------------------------------------------

struct MessageBoxConfig
{
	MsgButton	show_buttons;
	const char * ok_text;
	const char * cancel_text;
	const char * title_text;
};

using MessageBoxCallback = std::function<void(MsgButton)>;

//----------------------------------------------------------------------------------------------------------------------

class MessageBoxLayer : public CommonTitleLayer
{
public:
    virtual bool init(const char * msg, const MessageBoxCallback & cb, const MessageBoxConfig * cfg = nullptr);
    
	MessageBoxLayer();
	~MessageBoxLayer();

protected:
	cocos2d::ui::Text * labelMessage = nullptr;
	cocos2d::ui::Button * buttonOK = nullptr;
	cocos2d::ui::Button * buttonCancel = nullptr;

	void onClose();
	void onOKButtonClicked();
	void onCancelButtonClicked();

private:
	MessageBoxCallback m_Callback;
};

//----------------------------------------------------------------------------------------------------------------------

MessageBoxLayer * showMessage_CB_(const char * msg, const MessageBoxCallback & cb,
	const MessageBoxConfig * cfg, cocos2d::Scene * pScene = nullptr);

inline
MessageBoxLayer * showMessage_CB(const char * msg, cocos2d::Scene * pScene = nullptr)
{
	return showMessage_CB_(msg, MessageBoxCallback(), nullptr, pScene);
}
inline
MessageBoxLayer * showMessage_CB(const char * msg, const MessageBoxConfig & cfg, cocos2d::Scene * pScene = nullptr)
{
	return showMessage_CB_(msg, MessageBoxCallback(), &cfg, pScene);
}

inline
MessageBoxLayer * showMessage_CB(const char * msg, const MessageBoxCallback & cb, cocos2d::Scene * pScene = nullptr)
{
	return showMessage_CB_(msg, cb, nullptr, pScene);
}
inline
MessageBoxLayer * showMessage_CB(const char * msg, const MessageBoxCallback & cb,
	const MessageBoxConfig & cfg, cocos2d::Scene * pScene = nullptr)
{
	return showMessage_CB_(msg, cb, &cfg, pScene);
}

//----------------------------------------------------------------------------------------------------------------------

resumef::awaitable_t<MsgButton>
showMessage_(const char * msg, const MessageBoxConfig * cfg, cocos2d::Scene * pScene = nullptr);

inline
resumef::awaitable_t<MsgButton>
showMessage(const char * msg)
{
	return showMessage_(msg, nullptr, nullptr);
}
inline
resumef::awaitable_t<MsgButton>
showMessage(const char * msg, const MessageBoxConfig & cfg, cocos2d::Scene * pScene = nullptr)
{
	return showMessage_(msg, &cfg, pScene);
}
