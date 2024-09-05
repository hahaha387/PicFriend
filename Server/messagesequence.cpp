#include "messagesequence.h"

MessageSequence* MessageSequence::m_messageSequence=NULL;

MessageSequence *MessageSequence::getInstance()
{
    if(m_messageSequence==nullptr){
        m_messageSequence=new MessageSequence();
    }
    return m_messageSequence;
}

void MessageSequence::pushNotification(JottingNotification newNotification)
{
    std::cout<<"将消息加入到消息队列"<<std::endl;
    //加入消息给消息队列
    m_messageQueue.insert({newNotification.id(),newNotification});
}

void MessageSequence::removeNotification(std::string id)
{
    m_messageQueue.erase(id);

    std::string com_relation="delete from JottingNotificationRelation where JN_id="+id;
    RelationalBroker::drop(com_relation);

    std::string com="delete from JottingNotification where JN_id="+id;
    RelationalBroker::drop(com);
}

JottingNotification *MessageSequence::findById(std::string id)
{
    return &m_messageQueue.at(id);
}

void MessageSequence::updateMessageQueue(std::string netizenId)
{
    //发送消息队列中所有与netizen有关的message
    for(auto& message:m_messageQueue){
        message.second.notify(netizenId);
    }
}

void MessageSequence::removeMessageSubscriber(std::string messageId,std::string subscriberId)
{
    //删除队列中指定message的某个订阅者id（已查阅该message的netizen的id）
    m_messageQueue.at(messageId).removeSubscriber(subscriberId);

    //判断该消息是否全部订阅者查阅完毕
    if(m_messageQueue.at(messageId).isReadByAll()){
        std::cout<<"is empty"<<std::endl;
        //若全部查阅完毕，则删除该消息
        removeNotification(messageId);
    }
}

MessageSequence::~MessageSequence()
{

}

MessageSequence::MessageSequence()
{
    m_messageQueue.clear();
}
