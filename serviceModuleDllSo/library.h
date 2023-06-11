#ifndef MYDLLSO_LIBRARY_H
#define MYDLLSO_LIBRARY_H

#if (defined(_WIN32) || defined(_WIN64))
#if defined(SMMS_EXPORTS)
#define SMMS_EXTERN extern "C" __declspec(dllexport)
#else
#define SMMS_EXTERN extern "C" __declspec(dllimport)
#endif
#define SMMS_API __stdcall
#elif defined(__linux__)
#define SMMS_EXTERN extern "C"
#define SMMS_API
#else
#define SMMS_EXTERN
#define SMMS_API
#endif

/** @fn SMMS_EXTERN int SMMS_API SMMS_Init()
 *  @brief 初始化。
 *  @param void
 *  @return 成功返回0，否则返回其他值。
 */
SMMS_EXTERN int SMMS_API SMMS_Init();

/** @fn SMMS_EXTERN int SMMS_API SMMS_Uninit()
 *  @brief 反初始化。
 *  @param void
 *  @return void
 */
SMMS_EXTERN void SMMS_API SMMS_Uninit();

/** @fn SMMS_EXTERN int SMMS_API SMMS_Start()
 *  @brief 启动服务。
 *  @param void
 *  @return 成功返回0，否则返回其他值。
 */
SMMS_EXTERN int SMMS_API SMMS_Start();

/** @fn SMMS_EXTERN int SMMS_API SMMS_Stop()
 *  @brief 停止服务。
 *  @param void
 *  @return void
 */
SMMS_EXTERN void SMMS_API SMMS_Stop();

/** @fn SMMS_EXTERN int SMMS_API SMMS_GetLastError()
 *  @brief 获取错误码。
 *  @param void
 *  @return 返回错误码。
 */
SMMS_EXTERN int SMMS_API SMMS_GetLastError();

/** @fn SMMS_EXTERN const char* SMMS_API SMMS_GetVersion()
 *  @brief 获取版本信息。
 *  @param void
 *  @return 返回版本信息。
 */
SMMS_EXTERN const char *SMMS_API SMMS_GetVersion();

/** @fn SMMS_EXTERN int SMMS_API SMMS_Gtest(int argc, char* argv[])
 *  @brief 进行单元测试。
 *  @param void
 *  @return 成功返回0，否则返回其他值。
 */
SMMS_EXTERN int SMMS_API SMMS_Gtest(int argc, char *argv[]);
#endif // MYDLLSO_LIBRARY_H
