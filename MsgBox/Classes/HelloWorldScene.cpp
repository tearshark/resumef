#include "stdafx.h"
#include "HelloWorldScene.h"
#include "MessageBoxLayer.h"

USING_NS_CC;

Scene* HelloWorld::createScene()
{
    return HelloWorld::create();
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !Scene::init() )
    {
        return false;
    }
    
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    /////////////////////////////
    // 2. add a menu item with "X" image, which is clicked to quit the program
    //    you may modify it.

    // add a "close" icon to exit the progress. it's an autorelease object
    auto closeItem = MenuItemImage::create(
                                           "CloseNormal.png",
                                           "CloseSelected.png",
                                           CC_CALLBACK_1(HelloWorld::menuCloseCallback, this));
    
    closeItem->setPosition(Vec2(origin.x + visibleSize.width - closeItem->getContentSize().width/2 ,
                                origin.y + closeItem->getContentSize().height/2));

    // create menu, it's an autorelease object
    auto menu = Menu::create(closeItem, NULL);
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu, 1);

    /////////////////////////////
    // 3. add your codes below...

    // add a label shows "Hello World"
    // create and initialize a label
    
    auto label = Label::createWithTTF("Hello World", "fonts/Marker Felt.ttf", 24);
    
    // position the label on the center of the screen
    label->setPosition(Vec2(origin.x + visibleSize.width/2,
                            origin.y + visibleSize.height - label->getContentSize().height));

    // add the label as a child to this layer
    this->addChild(label, 1);

    // add "HelloWorld" splash screen"
    auto sprite = Sprite::create("HelloWorld.png");

    // position the sprite on the center of the screen
    sprite->setPosition(Vec2(visibleSize.width/2 + origin.x, visibleSize.height/2 + origin.y));

    // add the sprite as a child to this layer
    this->addChild(sprite, 0);

    return true;
}


void HelloWorld::menuCloseCallback(Ref* pSender)
{
    //Close the cocos2d-x game scene and quit the application
    Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}

void HelloWorld::onEnter()
{
	Scene::onEnter();
	this->showMessageBox();
}

void HelloWorld::showMessageBox_CB()
{
	showMessage_CB(u8"这是一条提示信息。\n点击'确认'来关闭游戏",
		[=](MsgButton ok)
		{
			if (ok == MsgButton::OK)
			{
				this->menuCloseCallback(nullptr);
			}
			else if(ok > MsgButton(0))
			{
				showMessage_CB(u8"您选择了留在游戏里。", [=](MsgButton)
				{
					CCLOG("end message box");
				});
			}
		}
	);
}

void HelloWorld::showMessageBox()
{
	GO
	{
		auto ok = co_await showMessage(u8"这是一条提示信息。\n点击'关闭'来关闭游戏");
		if (ok == MsgButton::OK)
		{
			this->menuCloseCallback(nullptr);
		}
		else
		{
			co_await showMessage(u8"您选择了留在游戏里。", {}, nullptr);
			CCLOG("end message box");
		}
	};
}

