/**
 * @Author: Conscien
 * @Description:
 * @DateTime: 2023/3/4 21:08
 * @Param:
 * @Return
 */

#include "library.h"
#include <iostream>

/** @fn SMMS_EXTERN int SMMS_API SMMS_Init()
 *  @brief 初始化。
 *  @param void
 *  @return 成功返回0，否则返回其他值。
 */
SMMS_EXTERN int SMMS_API SMMS_Init() {
  std::cout << "SMMS_Init is called" << std::endl;
  return 0;
}

/** @fn SMMS_EXTERN int SMMS_API SMMS_Uninit()
 *  @brief 反初始化。
 *  @param void
 *  @return void
 */
SMMS_EXTERN void SMMS_API SMMS_Uninit() {
  std::cout << "SMMS_Uninit is called" << std::endl;
}

/** @fn SMMS_EXTERN int SMMS_API SMMS_Start()
 *  @brief 启动服务。
 *  @param void
 *  @return 成功返回0，否则返回其他值。
 */
SMMS_EXTERN int SMMS_API SMMS_Start() {
  std::cout << "SMMS_Start is called" << std::endl;
  return 0;
}

/** @fn SMMS_EXTERN int SMMS_API SMMS_Stop()
 *  @brief 停止服务。
 *  @param void
 *  @return void
 */
SMMS_EXTERN void SMMS_API SMMS_Stop() {
  std::cout << "SMMS_Stop is called" << std::endl;
}

/** @fn SMMS_EXTERN int SMMS_API SMMS_GetLastError()
 *  @brief 获取错误码。
 *  @param void
 *  @return 返回错误码。
 */
SMMS_EXTERN int SMMS_API SMMS_GetLastError() {
  std::cout << "SMMS_GetLastError is called" << std::endl;
  return 0;
}

/** @fn SMMS_EXTERN const char* SMMS_API SMMS_GetVersion()
 *  @brief 获取版本信息。
 *  @param void
 *  @return 返回版本信息。
 */
SMMS_EXTERN const char *SMMS_API SMMS_GetVersion() {
  std::cout << "SMMS_GetVersion is called" << std::endl;
  return "V1.1.0.0(Build2023/03/04 00:00:00)";
}

/** @fn SMMS_EXTERN int SMMS_API SMMS_Gtest(int argc, char* argv[])
 *  @brief 进行单元测试。
 *  @param void
 *  @return 成功返回0，否则返回其他值。
 */
SMMS_EXTERN int SMMS_API SMMS_Gtest(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  std::cout << "SMMS_Gtest is called" << std::endl;
  return 0;
}