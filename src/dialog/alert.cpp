#include "alert.h"
#include <gui/builder.h>
#include <utility>

AlertDialog::AlertDialog(std::string title, std::string text, std::function<void(bool,bool)> onEnd, std::string truebtn, std::string falsebtn, uint32_t timeout, bool default_ret, bool dontshow):
	InfoDialog(title,text), onEnd(onEnd), timer(0), timeout(timeout), default_ret(default_ret), truebtn(truebtn), falsebtn(falsebtn), dontshowagain(dontshow)
{}

AlertDialog::AlertDialog(std::string title, std::vector<std::string_view> lines, std::function<void(bool,bool)> onEnd, std::string truebtn, std::string falsebtn, uint32_t timeout, bool default_ret, bool dontshow):
	InfoDialog(title,lines), onEnd(onEnd), timer(0), timeout(timeout), default_ret(default_ret), truebtn(truebtn), falsebtn(falsebtn), dontshowagain(dontshow)
{}

int32_t AlertDialog::alert_on_tick()
{
	if(timeout)
	{
		if(++timer > timeout)
		{
			onEnd(default_ret,dontshowagain);
			return ONTICK_EXIT;
		}
	}
	return ONTICK_CONTINUE;
}

std::shared_ptr<GUI::Widget> AlertDialog::view()
{
	using namespace GUI::Builder;
	using namespace GUI::Props;
	using namespace GUI::Key;
	bool dsa = dontshowagain;
	dontshowagain = false;
	
	std::shared_ptr<GUI::Widget> trueb = DummyWidget();
	std::shared_ptr<GUI::Widget> falseb = DummyWidget();
	if (truebtn.size())
		trueb = Button(
			text = truebtn,
			minwidth = 90_px,
			onClick = message::OK,
			focused = true
		);
	if (falsebtn.size())
		falseb = Button(
			text = falsebtn,
			minwidth = 90_px,
			onClick = message::CANCEL
		);
	
	return Window(
		title = std::move(dlgTitle),
		onClose = message::CANCEL,
		use_vsync = true,
		onTick = [&](){return alert_on_tick();},
		hPadding = 0_px,
		Column(
			hPadding = 0_px, 
			Label(noHLine = true,
				hPadding = 2_em,
				maxLines = 20,
				maxwidth = Size::pixels(zq_screen_w)-12_px-5_em,
				textAlign = 1,
				text = std::move(dlgText)),
			Checkbox(visible = dsa,
				text = "Don't show this message again",
				checked = false,
				onToggleFunc = [&](bool state)
				{
					dontshowagain = state;
				}
			),
			Row(
				topPadding = 0.5_em,
				vAlign = 1.0,
				spacing = 2_em,
				trueb,
				falseb
			)
		)
	);
}

bool AlertDialog::handleMessage(const GUI::DialogMessage<int32_t>& msg)
{
	onEnd(((message)msg.message)==message::OK,dontshowagain);
	return true;
}
