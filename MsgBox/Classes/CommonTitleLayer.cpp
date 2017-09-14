#include "stdafx.h"
#include "seek_widget_helper.inl"
#include "CommonTitleLayer.h"

USING_NS_CC;

bool CommonTitleLayer::init(const char* fileName, const char * titleText)
{
	if (!Layout::init())
		return false;

	this->uiLayer = cocostudio::GUIReader::shareReader()->widgetFromJsonFile(fileName);
	if (this->uiLayer == nullptr)
		return false;
	addChild(uiLayer);

	SeekAllWidget(uiLayer, labelTitle, buttonClose);

	if (this->labelTitle != nullptr && titleText)
		this->labelTitle->setString(titleText);
	if (this->buttonClose != nullptr)
		this->buttonClose->addClickEventListener(std::bind(&CommonTitleLayer::onClose, this));

	return true;
}

void CommonTitleLayer::onClose()
{
	this->removeFromParent();
}
