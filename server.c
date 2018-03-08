#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>

#define MAX_CLIENT 2

// オセロ用構造体
typedef struct othelloInfo{
	int pair[2]; // pair[0]==先手(黒), pair[1]==後手(白)
	int table[8][8];  // -1==black, 0==none, 1=white
	int allowed[8][8];
	int turn;  // -1==black, 1==white
	int count;
}othelloInfo;

// オセロ用変数
typedef struct coordinate{
	int x;
	int y;
}coordinate;

// グローバル変数
othelloInfo othello;
coordinate place;
bool lock_start;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// 最大接続数を超えた場合にクライアントに謝りを入れる関数
void sorry(int *new_sockfd)
{
	int fd = *new_sockfd;
	int len;
	char buf[32];

	sprintf(buf,"Sorry, please wait.\n");

	len = send(fd, buf, strlen(buf), 0);

	pthread_exit(buf);
}

// その位置に打った時、ひっくり返すコマがあるか判定する関数
bool canPutDown(int vecX, int vecY) {

	int x = place.x;
	int y = place.y;

	// 隣の場所へ、どの隣かは(vecX, vecY)が決める。
	x += vecX;
	y += vecY;

	// 隣が自分の石の場合は打てない
	if (othello.table[x][y] == othello.turn){
		return false;
	}
	// 隣が空白の場合は打てない
	if (othello.table[x][y] == 0){
		return false;
	}

	// さらに隣を調べていく
	x += vecX;
	y += vecY;
	// となりに石がある間ループがまわる
	while (x >= 0 && x < 8 && y >= 0 && y < 8) {
		// 空白が見つかったら打てない（1つもはさめないから）
		if (othello.table[x][y] == 0){
			return false;
		}
		// 自分の石があればはさめるので打てる
		if (othello.table[x][y] == othello.turn){
			return true;
		}
		x += vecX;
		y += vecY;
	}
	// 相手の石しかない場合はいずれ盤面の外にでてしまうのでfalse
	return false;
}

// クライアントの入力エラーをチェックする関数
char *error_check(char *buf){
	bool flag = false;

	// 入力エラー確認
	if ( place.x < 0 || place.x > 7 ){
		fprintf(stderr,"error: 1<=x<=8\n");
		return "error: 1<=x<=8";
	}
	if ( buf[1] != ',' ){
		fprintf(stderr,"error: x,y\n");
		return "error: x,y";
	}
	if ( place.y < 0 || place.y > 7 ){
		fprintf(stderr,"error: 1<=y<=8\n");
		return "error: 1<=y<=8";
	}

	/***********************
	* ここにエラー処理追加
	************************/
	// コマがあるかないか
	if( othello.table[place.x][place.y]!=0 ){
		fprintf(stderr,"error: already exists\n");
		return "error: already exists";
	}
	// ひっくり返せるコマがあるかないか
	if (canPutDown(1, 0))
	flag = true; // 右
	if (canPutDown(0, 1))
	flag = true; // 下
	if (canPutDown(-1, 0))
	flag = true; // 左
	if (canPutDown(0, -1))
	flag = true; // 上
	if (canPutDown(1, 1))
	flag = true; // 右下
	if (canPutDown(-1, -1))
	flag = true; // 左上
	if (canPutDown(1, -1))
	flag = true; // 右上
	if (canPutDown(-1, 1))
	flag = true; // 左下

	if (flag == false){
		fprintf(stderr, "error: There is no stone that can be invert.\n");
		return "error: There is no stone that can be invert.";
	}
	else
	return "OK";
}

// オセロ盤を描画する関数
char *drawing(char *bufMes){
	int i, j;

	sprintf(bufMes, "\nturn: %d\n  1 2 3 4 5 6 7 8 x\n +---------------+\n",othello.count);
	for ( i=0 ; i<8 ; i++ ){
		sprintf(bufMes, "%s%d", bufMes, i+1);
		for ( j=0 ; j<8 ; j++ ){
			sprintf(bufMes, "%s|", bufMes);
			switch (othello.table[i][j]){
				case 0:
				sprintf(bufMes, "%s ", bufMes);
				break;
				case 1:
				sprintf(bufMes, "%s●", bufMes);
				break;
				case -1:
				sprintf(bufMes, "%s○", bufMes);
				break;
			}
		}
		sprintf(bufMes, "%s|\n", bufMes);
	}
	sprintf(bufMes, "%sy+---------------+\n",bufMes);

	return bufMes;
}

// ひっくり返す(処理)
void reverseProcess(int vecX, int vecY){
	int x = place.x;
	int y = place.y;
	//printf("callreverseProcess\n");
	x += vecX;
	y += vecY;

	while (othello.table[x][y] != othello.turn) {
		// ひっくり返す
		// printf("vecx=%d, vect=%d, table[%d][%d]: reverse\n",vecX,vecY,x,y);
		othello.table[x][y] = othello.turn;
		x += vecX;
		y += vecY;
	}
}

// ひっくり返す(判定)
void reverse(){
	// printf("callreverse\n");
	if (canPutDown(1, 0))
	    reverseProcess(1, 0); // 右
	if (canPutDown(0, 1))
	    reverseProcess(0, 1); // 下
	if (canPutDown(-1, 0))
	    reverseProcess(-1, 0); // 左
	if (canPutDown(0, -1))
	    reverseProcess(0, -1); // 上
	if (canPutDown(1, 1))
	    reverseProcess(1, 1); // 右下
	if (canPutDown(-1, -1))
	    reverseProcess(-1, -1); // 左上
	if (canPutDown(1, -1))
	    reverseProcess(1, -1); // 右上
	if (canPutDown(-1, 1))
	    reverseProcess(-1, 1); // 左下
}

// スレッド: メイン処理
void do_thread(int *new_sockfd)
{
	int fd = *new_sockfd;
	int fdIndex;
	int myColorNum; // -1==black, 1=white
	int len, i, j, tempTurn;
	char myColor[6];
	char buf[BUFSIZ] = {0};
	char *buf2;
	char bufMes[1024] = {0};

	// fdの情報を元に、このスレッドは白か黒かをフィールドに登録
	if ( othello.pair[0] == fd ){
		fdIndex = 0;
		myColorNum = -1;
		sprintf(myColor, "black");
	}
	if ( othello.pair[1] == fd ){
		fdIndex = 1;
		myColorNum = 1;
		sprintf(myColor, "white");
	}

	// ゲームループ
	while( strncasecmp(buf, "exit\n", 5) != 0 ){

		// 相手からシグナルがくるまでwait
		if ( lock_start == true ){
			pthread_mutex_lock(&mutex);
			pthread_cond_wait(&cond, &mutex);
		}

		lock_start = true;
		memset(bufMes, 0, 1024);

		/* drawing、送信 */
		sprintf(bufMes, "%s",drawing(bufMes));
		sprintf(bufMes, "%sIt's your turn(%s). Please put coordinate(x,y).\nx,y = \n", bufMes, myColor);
		len = send(fd, bufMes, strlen(bufMes), 0);

		// メッセージ受信
		len = recv(fd, buf, BUFSIZ, 0);

		// 座標読み取り
		place.y = buf[0] - '1';
		place.x = buf[2] - '1';

		// 受信メッセージの処理
		buf2 = error_check(buf);

		// 受信メッセージが適合しないなら、クライアントにエラーを返してもう一度入力させる
		while( strcmp(buf2,"OK") ){
			len = send(fd, buf2, strlen(buf2), 0);
			memset(bufMes, 0, 1024);
			sprintf(bufMes, "\nPlease input x,y = \n");
			len = send(fd, bufMes, strlen(bufMes), 0);
			len = recv(fd, buf, BUFSIZ, 0);
			// 座標読み取り
			place.y = buf[0] - '1';
			place.x = buf[2] - '1';
			// 受信メッセージの処理
			buf2 = error_check(buf);
		}

		// 石配置
		othello.table[place.x][place.y] = othello.turn;

		// ひっくり返す
		reverse();

		// 盤面の処理(次のターンへ移行するため)
		othello.turn *= -1;
		othello.count++;

		/* drawing */
		memset(bufMes, 0, 1024);
		sprintf(bufMes, "%s", drawing(bufMes));
		sprintf(bufMes, "%sIt's not your turn. Please wait.\n\n", bufMes);
		len = send(fd, bufMes, strlen(bufMes), 0);

		/***********************
		* ここにゲーム終了判定処理(未完)
		************************/

		// 自ターン終了 -> 相手のスレッドにシグナル送信
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);

	}
}


int main(int argc, char *argv[])
{
	// ソケット関連
	int sockfd, len, new_sockfd, port, i=0;
	char buf[BUFSIZ];
	char *buf2;
	struct sockaddr_in serv, clnt;
	socklen_t sin_siz;
	pthread_t td;

	// オセロ用
	int x, y;

	// オセロ盤初期化
	for ( x=0 ; x<8 ; x++ ){
		for ( y=0 ; y<8 ; y++ ){
			othello.table[x][y] = 0;
		}
	}
	othello.table[3][3] = othello.table[4][4] = 1;
	othello.table[3][4] = othello.table[4][3] = -1;
	othello.turn = -1;
	othello.count=0;
	lock_start = false;

	// usage
	if ( argc != 3 ){
		printf("Usage: ./prog host port\n");
		exit(1);
	}

	/* server */
	if ( (sockfd = socket(PF_INET,SOCK_STREAM, 0) ) <0 ){
		perror("socket");
		exit(1);
	}

	serv.sin_family = PF_INET;
	port = strtol(argv[2], NULL, 10);
	serv.sin_port = htons(port);
	inet_aton(argv[1], &serv.sin_addr);
	sin_siz = sizeof(struct sockaddr_in);

	if ( bind(sockfd, (struct sockaddr *)&serv, sizeof(serv)) <0 ){
		perror("bind");
		exit(1);
	}

	if ( listen(sockfd, SOMAXCONN) <0 ){
		perror("listen");
		exit(1);
	}

	while(1){

		if ( (new_sockfd = accept(sockfd, (struct sockaddr *) &clnt, &sin_siz)) <0 ){
			perror("accept");
			exit(1);
		}

		printf("connect from %s: %d\n",inet_ntoa(clnt.sin_addr), ntohs(clnt.sin_port));

		// MAX_CILENT以上のクライアントの接続時、sorry関数へ
		if ( i>=MAX_CLIENT ){
			if (pthread_create(&td, NULL, (void *)sorry, &new_sockfd) != 0) {
				perror("pthread_create");
				exit(1);
			}
			continue;
		}

		// MAX_CILENT以下のクライアントの接続時、do_thread関数へ
		else{
			othello.pair[i] = new_sockfd;
			if (pthread_create(&td, NULL, (void *)do_thread, &new_sockfd) != 0) {
				perror("pthread_create");
				exit(1);
			}
		}
		pthread_detach(td);
		i++;
	}
	pthread_join(td,NULL);
	close(sockfd);
	close(new_sockfd);
	return 0;
}
