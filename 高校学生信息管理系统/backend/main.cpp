// 高校学生信息管理系统 后端入口
//
// 所有 Controller 通过 METHOD_LIST 注册路由（HttpController<T> 默认
// AutoCreation=true，框架启动时自动实例化并挂载路由），
// JwtFilter 以 "JwtFilter" 名称挂载到需要鉴权的接口上，
// 因此 main 无需手动注册任何控制器，只需加载配置并启动。

#include <drogon/drogon.h>

int main()
{
    // 加载端口 / MySQL / JWT 密钥配置（工作目录须为 backend/）
    drogon::app().loadConfigFile("config/app.json");

    LOG_INFO << "====================================";
    LOG_INFO << " 高校学生信息管理系统后端启动";
    LOG_INFO << " 地址: http://localhost:8080";
    LOG_INFO << "====================================";

    drogon::app().run();
    return 0;
}
