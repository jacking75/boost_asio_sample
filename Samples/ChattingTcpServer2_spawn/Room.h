#pragma once


#include <set>

class chat_room
{
	typedef std::shared_ptr<chat_participant> chat_participant_ptr;
	typedef std::deque<chat_message> chat_message_queue;

public:
	void join(chat_participant_ptr participant)
	{
		participants_.insert(participant);
		
		for (auto msg : recent_msgs_)
		{
			participant->deliver(msg);
		}
	}

	void leave(chat_participant_ptr participant)
	{
		participants_.erase(participant);
	}

	void deliver(const chat_message& msg)
	{
		recent_msgs_.push_back(msg);
		
		while (recent_msgs_.size() > max_recent_msgs)
		{
			recent_msgs_.pop_front();
		}

		for (auto participant : participants_)
		{
			participant->deliver(msg);
		}
	}


private:
	std::set<chat_participant_ptr> participants_;
	enum { max_recent_msgs = 100 };
	chat_message_queue recent_msgs_;
};