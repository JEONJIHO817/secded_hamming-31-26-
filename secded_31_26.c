#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


// https://graphics.stanford.edu/~seander/bithacks.html#ParityParallel
static inline bool parity32(uint32_t v)                             //각 패리티 비트 자리를 계산하는 함수
{
	v ^= v >> 16;
	v ^= v >> 8;
	v ^= v >> 4;
	v &= 0xf;
	return (0x6996 >> v) & 1;
}

// Hamming(31, 26) plus total parity at bit 0 for double error detection
// https://en.wikipedia.org/wiki/Hamming_code#General_algorithm
uint32_t hamming_encode(uint32_t d)                                 //v를 데이터로 받아서 인코딩하는 함수
{
	// move data bits into position
	uint32_t h =
		(d &                       1) << 3 |                        //데이터 비트의 첫번째 비트를 코드의 4번째 비트에 둠
		(d & ((1 <<  4) - (1 <<  1))) << 4 |                        //데이터 비트의 2~4번째 비트를 코드의 6~8번째 비트에 둠
		(d & ((1 << 11) - (1 <<  4))) << 5 |                        //데이터 비트의 5~11번째 비트를 코드의 10~16번째 비트에 둠
		(d & ((1 << 26) - (1 << 11))) << 6;                         //데이터 비트의 12~26번째 비트를 코드의 18~32번째 비트에 둠
	// compute parity bits
	h |=
		parity32(h & 0b10101010101010101010101010101010) << 1 |     //해당 자리에 1의 갯수가 짝수개인지 검사해서 P1자리에 넣음
		parity32(h & 0b11001100110011001100110011001100) << 2 |     //해당 자리에 1의 갯수가 짝수개인지 검사해서 P2자리에 넣음
		parity32(h & 0b11110000111100001111000011110000) << 4 |     //해당 자리에 1의 갯수가 짝수개인지 검사해서 P4자리에 넣음
		parity32(h & 0b11111111000000001111111100000000) << 8 |     //해당 자리에 1의 갯수가 짝수개인지 검사해서 P8자리에 넣음
		parity32(h & 0b11111111111111110000000000000000) << 16;     //해당 자리에 1의 갯수가 짝수개인지 검사해서 P16자리에 넣음
	// overall parity
	return h | parity32(h);                                         //짝수 개의 오류를 검출하기 위해 최하위 비트에 전체 패리티 값 넣음
}

uint32_t hamming_decode(uint32_t h)
{
	// overall parity error
	bool p = parity32(h);                                           //h 전체의 패리티 값을 p에 저장
	// error syndrome
	uint32_t i =
		parity32(h & 0b10101010101010101010101010101010) << 0 |     //i값이 신드롬 값이고 각 패리티 비트 별로 계산해서 i에 넣음
		parity32(h & 0b11001100110011001100110011001100) << 1 |
		parity32(h & 0b11110000111100001111000011110000) << 2 |
		parity32(h & 0b11111111000000001111111100000000) << 3 |
		parity32(h & 0b11111111111111110000000000000000) << 4;
	// correct single error or detect double error
	if (i != 0) {                                                   //신드롬 값이 0이 아니면 에러가 발생한 것으로 판단
		if (p == 1) { // single error                               //이때 전체 패리티 값이 true라면 1의 갯수가 홀수라는 것이므로 신드롬 값인 i번째에 비트를 뒤집음으로써 오류를 정정해줌
			h ^= 1 << i;
		} else { // double error                                    //전체 패리티 값이 false라면 1의 갯수가 짝수라는 것이므로 오류가 있음에도 1의 갯수가 짝수라면 더블 에러로 판단후 -1반환
			return -1;
		}
	}
	// remove parity bits
	return
		((h >> 3) & 1) |
		((h >> 4) & ((1 <<  4) - (1 <<  1))) |
		((h >> 5) & ((1 << 11) - (1 <<  4))) |
		((h >> 6) & ((1 << 26) - (1 << 11)));                       //싱글 에러를 정정한 코드나 에러가 없는 코드의 데이터 비트만 추출 후 반환
}

void print_bin(uint32_t v)                          //2진수로 변환하여 출력하는 함수
{
	for (int i = 31; i >=0; i--)
		printf("%d", (v & (1 << i)) >> i);
}

int main(int argc, char **argv)
{
	uint32_t v = (argc < 2) ? 12345678 : atoi(argv[1]);     //v변수 선언 및 초기화
	if (v >= (1 << 26)) {                                   //데이터 비트 = 26비트 이므로 26비트 이상이면 인코딩 불가
		printf("use a smaller number < %d\n", (1 << 26));
		return 0;
	}
	printf("value: %d\n", v);
	printf("binary:\n");
	printf("\t      dddddddddddddddddddddddddd\n");
	printf("\t"); print_bin(v); printf("\n");
	uint32_t h = hamming_encode(v);
	printf("encoded:\n");
	printf("\tddddddddddddddd ddddddd ddd d\n");
	printf("\t"); print_bin(h); printf("\n");
	printf("\t               p       p   p ppp\n");

	printf("single bit errors:\n");
	for (int i = 0; i < 32; i++) {
		if (hamming_decode(h ^ (1 << i)) != v) {                            //0번째부터 31번째 비트까지 한 비트에 오류가 발생한 상황을 가정하여 디코딩 함수에 넣음
			printf("single bit error correction fail: %d, %d", v, i);       //한 비트의 오류를 정정하지 못하여 기존 값과 다른 값으로 디코딩 됐을 때 오류 메세지 출력 후 프로그램 종료
			return 0;
		}
	}
	printf("\tpassed\n");

	printf("double bit errors:\n");
	for (int j = 0; j < 32; j++) {                                          //두 비트에 오류가 발생하는 모든 경우의 수를 가정하여 디코딩 함수에 넣음
		for (int i = 0; i < j; i++) {                                       //더블 에러를 검출하지 못하면 오류 메세지 출력 후 프로그램 종료
			if (hamming_decode(h ^ (1 << i) ^ (1 << j)) != -1) {
				printf("double bit error detection fail: %d, %d, %d\n", v, i, j);
				return 0;
			}
		}
	}
	printf("\tpassed\n");
}
