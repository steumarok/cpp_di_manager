#include <vector>
#include <memory>
#include "lib/di_manager.hpp"

using namespace di_manager;

//
// Global logging service
//
class LoggingService 
{
public:
    LoggingService() 
    {
        std::cout << "LoggingService" <<std::endl;
    }

    ~LoggingService() 
    {
        std::cout << "~LoggingService" <<std::endl;
    }

    void log(const std::string& msg) 
    {
        std::cout << msg << std::endl;
    }
};

//
// Unregistered service
//
class ExternalService 
{
public:
    void foo()
    {
        std::cout << "foo" << std::endl;
    }
};

//
// Request scoped service
//
class UserService 
{
    //
    // Injecting global logger
    //
    [[=Inject{}]]
    LoggingService* logger;

    [[=Inject{}]]
    std::shared_ptr<ExternalService> extSvc;

public:
    void createUser()
    {
        logger->log("Create User");

        extSvc->foo();
    }
};

class Application;

class RequestHandler 
{
    //
    // Injected through contructor
    //
    UserService& userService_;

    Application& application_;

    std::string path_;

public:
    RequestHandler(
        std::string path,
        [[=Inject{}]] Application& application,
        [[=Inject{}]] UserService& userService
    )
    : path_(std::move(path))
    , application_(application)
    , userService_(userService)
    {        
    }

public:
    void process();
};


class Application 
{
public:
    virtual ~Application() 
    {
        logger->log("~Application");
    }

    [[=Inject{}]]
    LoggingService* logger;

    void handle(const std::string& path)
    {
        logger->log("handling " + path);

        addHandler(path);
    }

    std::string getName() const
    {
        return "App";
    }

protected:
    virtual void addHandler(const std::string& path) = 0;
};

class WebApplication : public Application
{
    std::vector<std::string> handlers_;

    //
    // Scoped transient object. The scope is returned to the caller.
    //
    [[=Inject{.transient=false}]]
    std::function<Scoped<RequestHandler*>(std::string path)> getRequestHandler2;

    [[=Inject{.transient=true}]]
    std::function<Scoped<std::unique_ptr<RequestHandler>>(std::string path)> createTransientRequestHandler;

    [[=Inject{.transient=false}]]
    Provider<RequestHandler*> getRequestHandler;

public:
    virtual ~WebApplication() 
    {
        std::cout << "~WebApplication" <<std::endl;
    }

    void process() 
    {
        for (const auto& path : handlers_)
        {
            auto scopedHandler = getRequestHandler(path);

            scopedHandler->process();
        }

        //requestHandler->process();
    }

protected:
    void addHandler(const std::string& path) override
    {
        handlers_.push_back(path);
    }
};


void RequestHandler::process()
{
    std::cout << "processing " << path_ << "in " << application_.getName() << std::endl;
    userService_.createUser();
}


int main()
{
    //
    // Scoped registry configuration.
    // Enabled policy for parent objects resolution, such as logger.
    // Fallback options are:
    //  - TryParent: try to resolve in the parent
    //  - TryParentOrCreate: try to resolve in parent or create if fail.
    //  - Create: always create the object
    //  - None: fail if can't resolve.
    //
    using RequestCfg = DefaultRegistryConfiguration::Extend<
        ResolutionPolicy<ResolutionFallback::TryParentOrCreate>
    >;

    //
    // Request classes registry.
    //
    using RequestRegistry = Registry<RequestCfg>
        ::add<UserService>;

    //
    // Root registry. Fail if try to inject a unknown class.
    // ResolutionFallback can be configured to allow use of 
    // unknown classes.
    //
    using RootCfg = DefaultRegistryConfiguration::Extend<
        ResolutionPolicy<ResolutionFallback::None>
    >;

    using RootRegistry = Registry<RootCfg>
        ::add<Application, WebApplication>
        ::add<RequestHandler, Configuration<NewContainer<RequestRegistry>>>
        ::add<LoggingService>;

    using RootContainer = Container<RootRegistry>;

    RootContainer root;
    
    //
    // Resolve root object and use.
    // Object lifetime is managed by underlying container.
    //
    auto& app = root.resolve<Application&, false>();
    app.handle("/test");

    static_cast<WebApplication&>(app).process();

    return 0;
}