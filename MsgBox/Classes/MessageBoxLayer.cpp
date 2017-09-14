#include "stdafx.h"
#include "seek_widget_helper.inl"
#include "MessageBoxLayer.h"

USING_NS_CC;

MessageBoxLayer::MessageBoxLayer()
{
}

MessageBoxLayer::~MessageBoxLayer()
{
	auto cb = std::move(m_Callback);
	if (cb)
		cb(MsgButton(0));
}

bool MessageBoxLayer::init(const char * msg, const MessageBoxCallback & cb, const MessageBoxConfig * cfg)
{
	if (!CommonTitleLayer::init("message_box/message_box.ExportJson", nullptr))
		return false;

	SeekAllWidget(uiLayer, labelMessage, buttonOK, buttonCancel);
	if (labelMessage == nullptr)
		return false;
	if (buttonOK == nullptr)
		return false;

	MsgButton show_buttons = cfg ? ((uint32_t)cfg->show_buttons == 0 ? MsgButton::OK : cfg->show_buttons) : MsgButton::Default;

	//标题栏
	if (labelTitle != nullptr && cfg && cfg->title_text)
		labelTitle->setString(cfg->title_text);
	if (buttonClose != nullptr)
		buttonClose->setVisible((show_buttons & MsgButton::Close) != MsgButton(0));

	//显示的信息
	labelMessage->setString(msg);

	//确认按钮
	buttonOK->addClickEventListener(std::bind(&MessageBoxLayer::onOKButtonClicked, this));
	buttonOK->setVisible((show_buttons & MsgButton::OK) != MsgButton(0));
	if ((show_buttons & MsgButton::OKCancel) == MsgButton::OK)
	{
		auto szParent = buttonOK->getParent()->getContentSize();
		auto szButton = buttonOK->getContentSize();
		buttonOK->setPositionX(ceilf(szParent.width * 0.5f));
	}
	if (cfg && cfg->ok_text)
		buttonOK->setTitleText(cfg->ok_text);

	//取消按钮
	if (buttonCancel != nullptr)
	{
		buttonCancel->addClickEventListener(std::bind(&MessageBoxLayer::onCancelButtonClicked, this));
		buttonCancel->setVisible((show_buttons & MsgButton::Cancel) != MsgButton(0));
		if (cfg && cfg->cancel_text)
			buttonCancel->setTitleText(cfg->cancel_text);
		if ((show_buttons & MsgButton::OKCancel) == MsgButton::Cancel)
		{
			auto szParent = buttonCancel->getParent()->getContentSize();
			auto szButton = buttonCancel->getContentSize();
			buttonCancel->setPositionX(ceilf(szParent.width * 0.5f));
		}
	}

	m_Callback = cb;
    
    return true;
}

void MessageBoxLayer::onClose()
{
	auto cb = std::move(m_Callback);
	this->removeFromParent();

	if (cb)
		cb(MsgButton::Close);
}

void MessageBoxLayer::onOKButtonClicked()
{
	auto cb = std::move(m_Callback);
	this->removeFromParent();

	if (cb)
		cb(MsgButton::OK);
}

void MessageBoxLayer::onCancelButtonClicked()
{
	auto cb = std::move(m_Callback);
	this->removeFromParent();

	if (cb)
		cb(MsgButton::Cancel);
}

//----------------------------------------------------------------------------------------------------------------------

MessageBoxLayer * showMessage_CB_(const char * msg, const MessageBoxCallback & cb,
	const MessageBoxConfig * cfg, cocos2d::Scene * pScene)
{
	if (pScene == nullptr)
		pScene = Director::getInstance()->getRunningScene();
	if (pScene == nullptr)
		return nullptr;

	MessageBoxLayer * layer = new MessageBoxLayer;
	if (!layer->init(msg, cb, cfg))
	{
		layer->release();
		return nullptr;
	}
	else
	{
		pScene->addChild(layer, 999999);
		layer->autorelease();

		return layer;
	}
}

resumef::future_t<MsgButton>
showMessage_(const char * msg, const MessageBoxConfig * cfg, cocos2d::Scene * pScene)
{
	resumef::promise_t<MsgButton> promise;
	
	auto box = showMessage_CB_(msg, [st = promise._state](MsgButton ok)
	{
		if (ok > MsgButton(0))
		{
			st->set_value(ok);
		}
		else
		{
			st->throw_with_nested(std::exception("showMessage:box destructed!", 0));
		}
	}, cfg, pScene);

	if (box == nullptr)
	{
		promise._state->throw_with_nested(std::exception("showMessage:create box failed.", 0));
	}

	return promise.get_future();
}

