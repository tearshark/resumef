#pragma once

class CommonTitleLayer : public cocos2d::ui::Layout
{
public:
	virtual bool init(const char* fileName, const char * titleText = nullptr);
	virtual void onClose();
protected:
	cocos2d::ui::Widget* uiLayer = nullptr;
	cocos2d::ui::Text * labelTitle = nullptr;
	cocos2d::ui::Button * buttonClose = nullptr;
};
