#include <Poco/Util/Application.h>
#include <Poco/Util/PropertyFileConfiguration.h>
#include <Poco/Util/LoggingConfigurator.h>
#include <Poco/Net/SMTPClientSession.h>
#include <Poco/Net/MailRecipient.h>
#include <Poco/Net/MailMessage.h>
#include <Poco/SharedPtr.h>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/PartSource.h>
#include <Poco/NumberFormatter.h>
#include <Poco/Net/POP3ClientSession.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/NumberParser.h>
#include <Poco/Net/StreamSocket.h>

#include <list>

#include <iostream>
#include <fstream>
#include <string>

#define DEBUG_POP3
//#define DEBUG_SMTP

#define CONFIGPATH "config.cfg"
#define LOGPATH "mailtest.log"
using namespace std;
using namespace Poco::Util;
using namespace Poco::Net;

class MainApp:public Application
{
public:
    int main(const std::vector<string> &args);
};

int MainApp::main(const std::vector<string> &args)
{
    ofstream logfile;
    logfile.open(LOGPATH,ios_base::out|ios_base::app);

    string SenderServer;
    string SenderLogin;
    string SenderMailAddress;
    string SenderPassword;
    int SenderServerPort;
    bool SenderuseTLS;

    string ReceiveServer;
    string ReceiveLogin;
    string ReceiveMailAddress;
    string ReceivePassword;
    int ReceiveServerPort;
    bool ReceiveuseTLS;

    string ErrorServer;
    string ErrorLogin;
    string ErrorMailAddress;
    string ErrorPassword;
    int ErrorServerPort;
    bool ErroruseTLS;

    string SendTestMessageTo;
    string SendErrorMessageTo;

    int retry;
    int threshold;
    int nowNumber;
    int DeliverTimeout;
    int errorCount=0;

    string circlepath;

    list<int>listSentNumbers;
    list<int>listMessageNumbers;


    try {
        PropertyFileConfiguration *configFile;
        configFile=new PropertyFileConfiguration(CONFIGPATH);

        SenderServer=configFile->getString("SenderServer");
        SenderLogin=configFile->getString("SenderLogin");
        SenderMailAddress=configFile->getString("SenderMailAddress");
        SenderPassword=configFile->getString("SenderPassword");
        SenderServerPort=configFile->getInt("SenderServerPort",465);
        SenderuseTLS=configFile->getBool("SenderTLS",true);


        ReceiveServer=configFile->getString("ReceiveServer");
        ReceiveLogin=configFile->getString("ReceiveLogin");
        ReceiveMailAddress=configFile->getString("ReceiveMailAddress");
        ReceivePassword=configFile->getString("ReceivePassword");
        ReceiveServerPort=configFile->getInt("ReceiveServerPort",995);
        ReceiveuseTLS=configFile->getBool("ReceiveTLS",true);

        ErrorServer=configFile->getString("ErrorServer");
        ErrorLogin=configFile->getString("ErrorLogin");
        ErrorMailAddress=configFile->getString("ErrorMailAddress");
        ErrorPassword=configFile->getString("ErrorPassword");
        ErrorServerPort=configFile->getInt("ErrorServerPort",465);
        ErroruseTLS=configFile->getBool("ErrorTLS",true);

        SendTestMessageTo=configFile->getString("SendTestMessageTo");
        SendErrorMessageTo=configFile->getString("SendErrorMessageTo");

        retry=configFile->getInt("Retry",3);
        threshold=configFile->getInt("Threshold",retry/2+1);
        DeliverTimeout=configFile->getInt("DeliverTimeout",30);

        circlepath=configFile->getString("circlePath","circle.log");

        configFile->release();
    }

    catch(Poco::NotFoundException ex)
    {
        logfile<<"Config \"config.cfg\" Read Exception: "<<ex.code()<<"; "<<ex.name()<<"; "<<ex.message()<<"; "<<ex.displayText()<<endl;
        return -10;
    }
    catch(Poco::FileNotFoundException ex)
    {
        logfile<<"Not Found file config.cfg: "<<ex.code()<<"; "<<ex.name()<<"; "<<ex.message()<<"; "<<ex.displayText()<<endl;
        return -5;
    }

    try
    {
        PropertyFileConfiguration *cicleConfig=new PropertyFileConfiguration(circlepath);
        nowNumber=cicleConfig->getInt("number",0);
        cicleConfig->setInt("number",nowNumber+retry);
        cicleConfig->save(circlepath);
        cicleConfig->release();
    }
    catch(Poco::FileNotFoundException ex)
    {
        logfile<<"Not Found file \"cicle.cfg\", recreating: "<<ex.code()<<"; "<<ex.name()<<"; "<<ex.message()<<"; "<<ex.displayText()<<endl;

        ofstream out(circlepath.c_str());
        out<<"number: "<<1<<endl;
        out.close();

        return -5;
    }
    catch(Poco::NotFoundException ex)
    {
        logfile<<"Config \"cicle.cfg\" Read Exception: "<<ex.code()<<"; "<<ex.name()<<"; "<<ex.message()<<"; "<<ex.displayText()<<endl;
        return -10;
    }


    //initializeSSL();
    //(Poco::Net::Context::Usage, const string&, const string&, const string&, Poco::Net::Context::VerificationMode, int, bool, const string&)
    Context::Ptr ptrContext;

    try{
        Poco::SharedPtr<InvalidCertificateHandler>ptrHandler=new AcceptCertificateHandler(false);
        string emptyString="";
        string acceptString="ALL:!ADH:!LOW:!EXP:!MD5@STRENGTH";
        ptrContext=new Context(Context::CLIENT_USE,emptyString,emptyString,emptyString,Context::VERIFY_NONE,9,true,acceptString);
        SSLManager::instance().initializeClient(0,ptrHandler,ptrContext);
    }
    catch(Poco::Exception ex)
    {
        logfile<<"SSL Exception\t"<<ex.message()<<endl;
    }


#ifndef DEBUG_POP3
    //Отправка тестовых сообщений с порядковым числом в теме сообщения на адрес SendTestMessageTo
    try{
        SocketAddress smtpAddress(SenderServer,SenderServerPort);
        SMTPClientSession *session;

        if(SenderuseTLS)
        {
            SecureStreamSocket socket(smtpAddress,ptrContext);
            session=new SMTPClientSession(socket);
        }
        else
        {
            StreamSocket socket(smtpAddress);
            session=new SMTPClientSession(socket);
        }

        session->open();

        session->login(SMTPClientSession::AUTH_LOGIN,SenderLogin,SenderPassword);

        MailRecipient recipient(MailRecipient::PRIMARY_RECIPIENT,SendTestMessageTo);
        for(int i=0;i<retry;i++) //Отправка такого количества сообщений, какое указано в конфиг файле.
        {

            MailMessage message;
            message.addRecipient(recipient);
            message.setSender(SenderMailAddress);
            string subject,messageText;
            messageText=subject=Poco::NumberFormatter::format(nowNumber+i);
            message.setSubject(subject);
            messageText="Тестовое сообщение. Необходимо для тестирования.";
            message.setContentType("text/plain; charset=UTF-8");
            message.setContent(messageText, MailMessage::ENCODING_8BIT);

            session->sendMessage(message);
            listSentNumbers.push_back(nowNumber+i);
            sleep(3); //немного задержки перед отправкой второго сообщения
        }
        session->close();

        if(session)
            delete session;
    }
    catch(Poco::Exception ex)
    {
        logfile<<nowNumber<<"\t"<<ex.message()<<endl;
    }


    sleep(DeliverTimeout);
#endif

#ifndef DEBUG_SMTP

    //Приём сообщений с адреса ReceiveMailAddress
    try
    {
        POP3ClientSession * pop3Session;
        SocketAddress pop3address(ReceiveServer,ReceiveServerPort);


        if(ReceiveuseTLS)
        {
            SecureStreamSocket pop3Socket(pop3address,ptrContext);

            pop3Session=new POP3ClientSession(pop3Socket);
        }
        else
        {
            StreamSocket pop3socket(pop3address);

            pop3Session=new POP3ClientSession(pop3socket);
        }

        //Авторизация на почтовом сервере
        pop3Session->login(ReceiveLogin,ReceivePassword);
        POP3ClientSession::MessageInfoVec listMessages;
        pop3Session->listMessages(listMessages);


        //Приём сообщений и запись в список listMessageNembers чисел из темы сообщения
        for(vector<POP3ClientSession::MessageInfo>::iterator itMessage=listMessages.begin();itMessage!=listMessages.end();itMessage++)
        {
            int messageId=((POP3ClientSession::MessageInfo)(*itMessage)).id;
            MessageHeader tempMessageHeader;
            pop3Session->retrieveHeader(messageId,tempMessageHeader);
            string mailSubject;

            if(tempMessageHeader.has("Subject"))
            {
                mailSubject=tempMessageHeader.get("Subject");
                int subjectNumber;
                if(Poco::NumberParser::tryParse(mailSubject,subjectNumber))
                {
                    //int subjectNumber=Poco::NumberParser::parse(mailSubject);
                    listMessageNumbers.push_back(subjectNumber);
                }
            }
        }
        for(vector<POP3ClientSession::MessageInfo>::iterator itMessage=listMessages.begin();itMessage!=listMessages.end();itMessage++)
        {
            pop3Session->deleteMessage(((POP3ClientSession::MessageInfo)(*itMessage)).id);
        }
        if(pop3Session)
            delete pop3Session;

    }
    catch(Poco::Exception ex)
    {
        logfile<<nowNumber<<" "<<ex.message()<<"; "<<ex.displayText()<<"; "<<ex.className()<<endl;
    }


    //Проверка, все ли сообщения были доставлены
    for(list<int>::iterator itSentNumbers=listSentNumbers.begin();itSentNumbers!=listSentNumbers.end();itSentNumbers++)
    {
        bool sovpad=false;
        for(list<int>::iterator itMessageNumber=listMessageNumbers.begin();itMessageNumber!=listMessageNumbers.end();itMessageNumber++)
        {
            if((*itMessageNumber)==(*itSentNumbers))
            {
                sovpad=true;
                break;
            }
        }

        if(!sovpad)
        {
            errorCount++;
        }
    }
    logfile<<"Step: "<<nowNumber<<";\tTranferErrorCount="<<errorCount<<endl;


    //Если число недоставленных сообщений больше порогового, то отсылается сообщение на SendErrorMessageTo с ящика ErrorMailAddress
    if(errorCount>=threshold)
    {
        //В настоящее время доступно только SSL
        try {
            SocketAddress smtpErrorAddress(ErrorServer,ErrorServerPort);
            SMTPClientSession *errorSession;

            if(ErroruseTLS)
            {
                SecureStreamSocket socket(smtpErrorAddress,ptrContext);
                errorSession=new SMTPClientSession(socket);
            }
            else
            {
                StreamSocket socket(smtpErrorAddress);
                errorSession=new SMTPClientSession(socket);
            }

            errorSession->open();

            errorSession->login(SMTPClientSession::AUTH_LOGIN,ErrorLogin,ErrorPassword);

            MailRecipient recipient(MailRecipient::PRIMARY_RECIPIENT,SendErrorMessageTo);

            MailMessage message;
            message.addRecipient(recipient);
            message.setSender(ErrorMailAddress);
            string subject,messageText;
            subject="Возможна ошибка в работе почтовой службы";

            messageText="Возможна ошибка в работе почтовой службы на шаге "+Poco::NumberFormatter::format(nowNumber)+" из "+Poco::NumberFormatter::format(retry)+
                    " тестовых сообщений доставлено "+Poco::NumberFormatter::format(retry-errorCount);
            message.setContentType("text/plain; charset=UTF-8");
            message.setContent(messageText, MailMessage::ENCODING_8BIT);
            message.setSubject(subject);

            errorSession->sendMessage(message);
            errorSession->close();

            if(errorSession)
                delete errorSession;
        }
        catch(Poco::Exception ex)
        {
            logfile<<nowNumber<<"; "<<ex.message()<<endl;
        }
    }

#endif
    logfile.close();
    cout << "Hello World!" << endl;
    return 0;
}

POCO_APP_MAIN(MainApp)
