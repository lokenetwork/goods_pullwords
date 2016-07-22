#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <zconf.h>
#include <cerrno>

using namespace cv;
using namespace std;
#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
//图片像素结构体
struct picture_pix {
    int B;
    int G;
    int R;

    int location_row; //该像素处于图像的第几行
    int start_offset; //该像素距离本行开头的距离
    bool is_word_pix; //是否是字体像素
    struct picture_pix *next_pix; //下一个像素
    struct picture_pix *prev_pix; //前一个像素

};


//把openc的矩阵转成咱们的结构体
int transform_to_mstrcut(Mat &I, struct picture_pix **pic_pix);

//todo，判断当前像素是否是字体像素，以后可能要根据前后的连续像素做判断，先封装成函数
int check_is_word_pix(struct picture_pix *backgound_pix, struct picture_pix *current_pix);

void *thread_check_top_big_bottom(void *arg);

void check_top_big_bottom(bool *top_big_bottom, int I_rows, int I_cols, struct picture_pix *first_pix);

struct thread_top_big_bottom_check_strcut {
    int start_index;
    int end_index;
    int *distance_array;
    bool *top_big_bottom;
    //条件，用于结果完成，通知
    pthread_cond_t * check_top_big_bottom_cond;
    int * tourch_min_center_calculate_num_count;
};
//二值化计算像素数量最少值，超过这阀值做中间计算才有价值。
int min_center_calculate_num = 10;

int main(int argc, char **argv) {

    if (argc < 2) {
        cout << "Not enough parameters" << endl;
        return -1;
    }

    Mat I, J;

    I = imread(argv[1], CV_LOAD_IMAGE_COLOR);
    //cout << "E = " << endl << " " << I << endl << endl;


    if (!I.data) {
        cout << "The image" << argv[1] << " could not be loaded." << endl;
        return -1;
    }

    struct picture_pix *first_pix;
    transform_to_mstrcut(I, &first_pix);

    //todo,二值计算要封装成函数
    bool top_big_bottom = true;
    //触碰到最小阀值的次数，必须触碰两次
    int tourch_min_center_calculate_num_count = 0;
    pthread_mutex_t check_top_big_bottom_cond_mtx = PTHREAD_MUTEX_INITIALIZER;
    //新建一个条件锁，等待被触发
    pthread_cond_t check_top_big_bottom_cond = PTHREAD_COND_INITIALIZER;
    //加锁
    pthread_mutex_lock(&check_top_big_bottom_cond_mtx);

    check_top_big_bottom(&top_big_bottom, I.rows, I.cols, first_pix,&check_top_big_bottom_cond,&tourch_min_center_calculate_num_count);

    //结果校验while，pthread_cond_wait可能被意外唤醒
    while (tourch_min_center_calculate_num_count < 2) {
        //阻塞等待子线程通知，触碰两次最小阀值
        pthread_cond_wait(&check_top_big_bottom_cond, &check_top_big_bottom_cond_mtx);
    }

    printf("It is end\n");

    sleep(1500);

    return 0;

}

//判断这个左距离是不是从大到小，是不是呈现对称分布。
void check_top_big_bottom(bool *top_big_bottom, int I_rows, int I_cols, struct picture_pix *first_pix,pthread_cond_t * check_top_big_bottom_cond,int *tourch_min_center_calculate_num_count) {
    struct picture_pix *temp_pix = first_pix;

    //获取左侧第一个字体像素相对于行首像素的偏移距离
    int left_distance_array[I_rows];

    int currnt_left_distance_index = 0;
    for (int i = 1; i <= I_rows * I_cols; ++i) {
        //找到一个左侧字体像素，立即跳到下一行
        if (true == temp_pix->is_word_pix) {

            left_distance_array[currnt_left_distance_index] = temp_pix->start_offset;
            currnt_left_distance_index++;
            int current_row = temp_pix->location_row;
            for (int j = 0; j < I_cols; ++j) {
                temp_pix = temp_pix->next_pix;

                //已经跳到下一行
                if (current_row != temp_pix->location_row) {
                    //返回
                    break;
                } else {
                    //注意，这里不是下一行 才加。
                    i++;
                }
            }
        } else {
            temp_pix = temp_pix->next_pix;
        }
    }
    struct thread_top_big_bottom_check_strcut *left_index_data = (struct thread_top_big_bottom_check_strcut *) malloc(
            sizeof(struct thread_top_big_bottom_check_strcut));
    left_index_data->start_index = 0;
    left_index_data->end_index = I_rows - 1;
    left_index_data->distance_array = left_distance_array;
    left_index_data->top_big_bottom = top_big_bottom;
    left_index_data->check_top_big_bottom_cond = check_top_big_bottom_cond;
    left_index_data->tourch_min_center_calculate_num_count = tourch_min_center_calculate_num_count;

    //start_index 跟end_index 产生变化
    pthread_t thread_id_check_left_top_big_bottom;
    int rest = pthread_create(&thread_id_check_left_top_big_bottom, NULL, thread_check_top_big_bottom, left_index_data);
    if (0 != rest) {
        handle_error_en(rest, "创建线程失败 ----\n");
    }

    //获取右侧第一个字体像素相对于行首像素的偏移距离
    //temp_pix 重新指向头指针。
    temp_pix = first_pix;
    int right_distance_array[I_rows];
    //从最后一个元素往回计算
    int currnt_right_distance_index = 0;
    for (int i = 1; i <= I_rows * I_cols; ++i)
    {
        //找到一个右侧字体像素，立即跳到上一行
        if (true == temp_pix->is_word_pix) {

            right_distance_array[currnt_right_distance_index] = I_cols-temp_pix->start_offset;
            currnt_right_distance_index++;
            int current_row = temp_pix->location_row;
            for (int j = 0; j < I_cols; ++j) {
                temp_pix = temp_pix->prev_pix;

                //已经跳到下一行
                if (current_row != temp_pix->location_row) {
                    //返回
                    break;
                } else {
                    //注意，这里不是下一行 才加。
                    i++;
                }
            }
        } else {
            temp_pix = temp_pix->prev_pix;
        }
    }

    //倒序一下这个数组
    for (int k = 0; k< (I_rows/2) ; k++) {
        int temp = right_distance_array[k];
        right_distance_array[k] = right_distance_array[I_rows-1-k];
        right_distance_array[I_rows-1-k] = temp;
    }

    struct thread_top_big_bottom_check_strcut *right_index_data = (struct thread_top_big_bottom_check_strcut *) malloc(
            sizeof(struct thread_top_big_bottom_check_strcut));
    right_index_data->start_index = 0;
    right_index_data->end_index = I_rows - 1;
    right_index_data->distance_array = right_distance_array;
    right_index_data->top_big_bottom = top_big_bottom;
    right_index_data->check_top_big_bottom_cond = check_top_big_bottom_cond;
    right_index_data->tourch_min_center_calculate_num_count = tourch_min_center_calculate_num_count;

    //start_index 跟end_index 产生变化
    pthread_t thread_id_check_right_top_big_bottom;
    int right_rest = pthread_create(&thread_id_check_right_top_big_bottom, NULL, thread_check_top_big_bottom, right_index_data);
    if (0 != right_rest) {
        handle_error_en(right_rest, "创建线程失败 ----\n");
    }

    //pthread_join(thread_id_check_left_top_big_bottom, NULL);
    pthread_join(thread_id_check_right_top_big_bottom, NULL);

}

void *thread_check_top_big_bottom(void *data) {

    struct thread_top_big_bottom_check_strcut *index_data = (struct thread_top_big_bottom_check_strcut *) data;
    float center_value;
    int next_start_index_1;
    int next_end_index_1;
    //判断是奇数还是偶数
    if (((index_data->end_index + 1) % 2) == 0) {
        center_value =
                (index_data->distance_array[(index_data->end_index / 2) - 1] +
                 index_data->distance_array[(index_data->end_index / 2)]) / 2;
        next_start_index_1 = index_data->start_index;
        next_end_index_1 = (index_data->end_index+1) / 2 - 1;
    } else {
        center_value = index_data->distance_array[(index_data->end_index) / 2];
        next_start_index_1 = index_data->start_index;
        next_end_index_1 = (index_data->end_index) / 2;

    }
    //start跟end之间超过5个像素再进入递归，5是阀值
    if (min_center_calculate_num <= (next_end_index_1 - next_start_index_1 - 1)) {
        int thread_rest;
        //初始化结构体， 进入递归
        struct thread_top_big_bottom_check_strcut *next_index_data_1 = (struct thread_top_big_bottom_check_strcut *) malloc(
                sizeof(struct thread_top_big_bottom_check_strcut));
        next_index_data_1->start_index = next_start_index_1;
        next_index_data_1->end_index = next_end_index_1;
        next_index_data_1->distance_array = index_data->distance_array;
        next_index_data_1->top_big_bottom = index_data->top_big_bottom;

        //thread_check_top_big_bottom(next_index_data_1);
        pthread_t thread_id;
        thread_rest = pthread_create(&thread_id, NULL, thread_check_top_big_bottom, next_index_data_1);
        if (0 != thread_rest) {
            handle_error_en(thread_rest, "pthread_create 3");
        }

        int next_start_index_2 = (index_data->end_index) / 2;
        int next_end_index_2 = index_data->end_index;

        //初始化结构体， 进入递归
        struct thread_top_big_bottom_check_strcut *next_index_data_2 = (struct thread_top_big_bottom_check_strcut *) malloc(
                sizeof(struct thread_top_big_bottom_check_strcut));
        next_index_data_2->start_index = next_start_index_2;
        next_index_data_2->end_index = next_end_index_2;
        next_index_data_2->distance_array = index_data->distance_array;
        next_index_data_2->top_big_bottom = index_data->top_big_bottom;

        //thread_check_top_big_bottom(next_index_data_2);
        pthread_t thread_id_2;
        int thread_rest_2 = pthread_create(&thread_id_2, NULL, thread_check_top_big_bottom, next_index_data_2);

        if (0 != thread_rest_2) {
            handle_error_en(thread_rest_2, "pthread_create 4");
        }

    }else{
        //触碰阀值
        __sync_fetch_and_add(index_data->tourch_min_center_calculate_num_count, 1);
        if( 2 == *(index_data->tourch_min_center_calculate_num_count)  ){
            pthread_cond_signal(index_data->check_top_big_bottom_cond);
        }
    }

    if (index_data->distance_array[index_data->end_index] > center_value ||
        index_data->distance_array[index_data->start_index] < center_value) {
        *((*index_data).top_big_bottom) = false;
    }
    free(index_data);

}

int transform_to_mstrcut(Mat &I, struct picture_pix **first_pix) {
    // accept only char type matrices
    CV_Assert(I.depth() == CV_8U);

    int channels = I.channels();

    int nRows = I.rows;
    int nCols = I.cols * channels;

    if (I.isContinuous()) {
        nCols *= nRows;
        nRows = 1;
        int i, j;
        uchar *p;
        int current_pix = 1; //现在遍历到那个像素了
        int pix_number = I.rows * I.cols; //全部的像素数量
        for (i = 0; i < nRows; ++i) {
            p = I.ptr<uchar>(i);
            //选取第一点作为背景色
            struct picture_pix background_pix;
            background_pix.B = p[0];
            background_pix.G = p[1];
            background_pix.R = p[2];

            struct picture_pix *pix_array[pix_number];

            for (int current_pix_index = 1; current_pix_index <= pix_number; ++current_pix_index) {

                pix_array[current_pix_index - 1] = (struct picture_pix *) malloc(sizeof(struct picture_pix));

                int B_index = current_pix_index * 3 - 3;
                int G_index = current_pix_index * 3 - 2;
                int R_index = current_pix_index * 3 - 1;


                pix_array[current_pix_index - 1]->B = p[B_index];
                pix_array[current_pix_index - 1]->G = p[G_index];
                pix_array[current_pix_index - 1]->R = p[R_index];

                float temp_location_row = (float) current_pix_index / (float) I.cols;
                pix_array[current_pix_index - 1]->location_row = (int) ceil(temp_location_row);

                pix_array[current_pix_index - 1]->start_offset =
                        current_pix_index - 1 - ((pix_array[current_pix_index - 1]->location_row - 1) * I.cols);
                check_is_word_pix(&background_pix, pix_array[current_pix_index - 1]);

            }
            //再循环一次做指针指向
            for (int current_pix_index = 0; current_pix_index < pix_number; ++current_pix_index) {
                //头尾指向
                if (0 == current_pix_index) {
                    pix_array[current_pix_index]->prev_pix = pix_array[pix_number - 1];
                } else {
                    pix_array[current_pix_index]->prev_pix = pix_array[current_pix_index - 1];
                }

                if ((pix_number - 1) == current_pix_index) {
                    pix_array[current_pix_index]->next_pix = pix_array[0];
                } else {
                    pix_array[current_pix_index]->next_pix = pix_array[current_pix_index + 1];
                }

            }
            *first_pix = pix_array[0];


        }
        //todo，非连续的图片暂时不处理
    } else {

    }

    return 0;
};

int check_is_word_pix(struct picture_pix *backgound_pix, struct picture_pix *current_pix) {
    //单色最大差距
    int single_max_gap = 80;
    //双色最大差距
    int double_max_gap = 50;
    //三色最大差距
    int three_max_gap = 30;

    int B_gap = abs(backgound_pix->B - current_pix->B);
    int G_gap = abs(backgound_pix->G - current_pix->G);
    int R_gap = abs(backgound_pix->R - current_pix->R);
    //先检测单色的差距
    if (B_gap > single_max_gap || G_gap > single_max_gap || R_gap > single_max_gap) {
        current_pix->is_word_pix = true;

    } else if ((B_gap > double_max_gap && G_gap > double_max_gap) ||
               (B_gap > double_max_gap && R_gap > double_max_gap) ||
               (G_gap > double_max_gap && R_gap > double_max_gap)) {
        current_pix->is_word_pix = true;
    } else if (B_gap > three_max_gap || G_gap > three_max_gap || R_gap > three_max_gap) {
        current_pix->is_word_pix = true;
    } else {
        current_pix->is_word_pix = false;
    };

    return 0;
};


