/*
 * ntg_types.h
 *
 *  Created on: Aug 25, 2015
 *      Author: tzh
 */

#ifndef NTG_TYPES_H_
#define NTG_TYPES_H_

//系统返回值
#define  NTG_OK          0
#define  NTG_ERROR      -1
#define  NTG_AGAIN      -2
#define  NTG_BUSY       -3
#define  NTG_DONE       -4
#define  NTG_DECLINED   -5 //declined 拒绝
#define  NTG_ABORT      -6













#if 1
#define NTG_TEST	1
#else

#endif

/* Miscellaneous constants */
#ifndef	PATH_MAX			/* should be in <limits.h> */
#define	PATH_MAX	1024	/* max # of characters in a pathname */
#endif

/* Following could be derived from SOMAXCONN in <sys/socket.h>, but many
   kernels still #define it as 5, while actually supporting many more */
#define	LISTENQ		1024	/* 2nd argument to listen() */

/* Miscellaneous constants */
#define	MAXLINE		4096	/* max text line length */
#define	BUFFSIZE	8192	/* buffer size for reads and writes */


#define MAX_BUFFER 2048//缓存区大小
#define MESSAGE_SIZE 1024//请求消息大小

#define URL_SIZE        1024 		/*URL的长度限制*/
#define USER_MAX     800  /*模拟的用户数*/


#define MAX_THREAD_NUM 40/*线程池中最大的线程数*/
//这些时间都是以秒为单为（毫秒）
#define UM_TIME         600     /*最大用户时间*/
#define UM_BROWSE_TIME 62000/*用户最大浏览时间,超过这一时间为离线*/

/*
 * 队列的扫描周期(单位为毫秒)
 */
#define OFF_CYCLE  100/*离线队列扫描周期*/
#define BR_CYCLE 1 /*浏览队列扫描周期*/

/*
 定义用户的属性类型的宏
 用户修改指定的用户属性
 */
#define P_ID  			1
#define P_TYPE  	2
#define P_STATE 	3
#define P_URL 		4
#define P_TIME		5

/*
 * 用户类型
 */
#define WEB_USER		1   /*web用户*/
#define STREAM_USER 2 /*流媒体用户*/

/*
 * 用户的状态
 */
#define	US_OFFLINE 	1/*用户的离线状态*/
#define US_BROWSE 	2/*用户的浏览状态*/
#define US_WAIT			3/*用户等待线程服务的状态*/
#define US_LIVE 			4/*用户与远程服务器交互的状态*/
/*LIVE状态的子状态*/
//根据情况再定

/*
 * 队列的类型
 */
#define Q_A		1 /*队列A是按时间的升序队列*/
#define Q_B 		2 /*队列B是等待线程服务的队列，采用先进先出策略*/



// Various reasons of error when getting a page
#define nbAnswers 16
enum Error
{
  success,
  noDNS,
  noConnection,
  forbiddenRobots,
  timeout,
  badType,
  tooBig,
  err30X,
  err40X,
  earlyStop,
  duplicate,
  fastRobots,
  fastNoConn,
  fastNoDns,
  tooDeep,
  urlDup
};

/*
 * 接收数据的类型
 */
enum data_type{
	DATA_HTTP,/*http数据*/
	DATA_RTSP,/*rtsp数据*/
};

#endif /* NTG_TYPES_H_ */
