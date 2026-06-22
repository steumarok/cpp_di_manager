#include <vector>
#include "lib/di_manager.hpp"

using namespace di_manager;

class LoggingService {

public:
    LoggingService() {
        std::cout << "LoggingService" <<std::endl;
    }
    ~LoggingService() {
        std::cout << "~LoggingService" <<std::endl;
    }

    void log(const std::string& msg) {
        std::cout << msg <<std::endl;
    }
};

class UserService 
{
    [[=Inject{}]]
    LoggingService* logger;

public:
    void createUser()
    {
        logger->log("Create User");
    }
};

class RequestHandler 
{
    UserService& userService_;

public:
    RequestHandler([[=Inject{}]] UserService& userService)
    : userService_(userService)
    {        
    }

public:

    void process()
    {
       userService_.createUser();
    }
};


class Application 
{
public:
    virtual ~Application() {
        logger->log("~Application");
    }

    [[=Inject{}]]
    LoggingService* logger;

    void handle(const std::string& path)
    {
        logger->log("handling " + path);

        addHandler(path);
    }

protected:
    virtual void addHandler(const std::string& path) = 0;
};

class WebApplication : public Application
{
    std::vector<std::string> handlers_;

    [[=Inject{.transient=true}]]
    std::function<Scoped<std::unique_ptr<RequestHandler>>()> getRequestHandler;

public:
    virtual ~WebApplication() {
        std::cout << "~WebApplication" <<std::endl;
    }

    void process() 
    {
        for (const auto& path : handlers_)
        {
            auto scopedHandler = getRequestHandler();

            scopedHandler->process();
        }
    }

protected:
    void addHandler(const std::string& path) override
    {
        handlers_.push_back(path);
    }
};



int main()
{
    using RequestCfg = DefaultRegistryConfiguration::Extend<
        ResolutionPolicy<ResolutionFallback::TryParent>
    >;

    using RequestRegistry = Registry<RequestCfg>
        ::add<UserService>;


    using RootCfg = DefaultRegistryConfiguration::Extend<
        ResolutionPolicy<ResolutionFallback::None>
    >;

    using RootRegistry = Registry<RootCfg>
        ::add<Application, WebApplication>
        ::add<RequestHandler, Configuration<NewContainer<RequestRegistry>>>
        ::add<LoggingService>;
    using RootContainer = Container<RootRegistry>;

    RootContainer root;
    
    auto* app = root.resolve<Application*>();
    app->handle("/test");

    static_cast<WebApplication*>(app)->process();

    return 0;
}