#include "tracker/loading.hpp"

#include "tracker/dualdvo.hpp"

static int N_FRAMES = 900;
static const int iterframes = 50;

extern const char* IMAGE_PATH;
extern const char* VIEW;
extern const char* ZBUF_SAVE_TEMP;

float host_img[w*h];
void loadZB(float*z, int sw, int sh, int num) {
    char name[100];
    sprintf(name, ZBUF_SAVE_TEMP, num, sw, sh);
    FILE*fp = fopen(name, "r");
    if (!fp) {
        cout << "File not found: " << name << endl;
        return;
    }
    int tmp;
    fscanf(fp, "P5\n%d %d\n%d\n", &sw, &sh, &tmp);
    unsigned short*data = new unsigned short[sw*sh];
    fread(data, 2, sw*sh, fp);
    for (int j = 0; j < sh; j++) {
        for (int i = 0; i < sw; i++) {
            unsigned short s = data[i+j*sw];
            int ind = sw*((h-sh)/2+j)+(w-sw)/2+i;
            z[ind] = s*(1.f/(1<<14))-1.f;
            if (z[ind] == -1.f || z[ind] < -0.5f) z[ind] = 0.f;
        }
    }
    delete[]data;
    fclose(fp);
}

float overlap(se3 a, se3 b) {
    se3 d = a*(b*-1);
    float trans[12];
    d.exp(trans, 0);
    float x = trans[2]+trans[3];
    float y = trans[6]+trans[7];
    float z = trans[10]+trans[11]-1;
    float dist = x*x+y*y+z*z;
    float theta = sqrt(d.nx*d.nx+d.ny*d.ny+d.nz*d.nz);
    return cos(theta)/dist;
}

void dumpPose(FILE*fp, se3 view) {
    float transform[12];
    view.exp(transform, 0);
    for (int k = 0; k < 3; k++) {
        for (int j = 0; j < 3; j++)
            fprintf(fp, " %f", transform[j+k*4]);
        fprintf(fp, " %f\n", transform[3+k*4]);
    }
    fprintf(fp, " %f %f %f %f\n\n", 0.0, 0.0, 0.0, 1.0);
}

int dualdvo_main(int argc, char**argv) {
    if (argc > 1) {
        sscanf(argv[1], "%d", &N_FRAMES);
    } else {
        cout << "Using " << N_FRAMES << " frames by default, change by running " << argv[0] << " <N frames>" << endl;
    }

    initCL();

#if defined DEBUG
    initVisual(w, h);
#endif

    timeval ta, tb;
    try {
        SimpleDVO dvo;

        std::vector<se3> path;
        path.push_back(se3(0,0,0,0,0,0));

        std::vector<DepthImage> frames;
        loadZB(host_img, 512, 424, 0);
        frames.push_back(dvo.createDepthImage(host_img));



        float Dcx = 255.559, Dcy = 207.671+(512-424)/2.f, Dpers = 365.463;

        for (int scale = 0; scale < scales; scale++) {
            float f = 1.f/(1<<scale);
            float scaledprojection[4] = {(Dcx+0.5f)*f-0.5f, (Dcy+0.5f)*f-0.5f, Dpers*f, 1.f/(f*Dpers)};

            queue.enqueueWriteBuffer(dvo.device_projection[scale], 0, 0, sizeof(float)*4, scaledprojection);
        }

        float transform[12];

        float Ab[29*iterframes*2];


        FILE*tmp_fp = fopen("tmp_views.txt", "w");
        dumpPose(tmp_fp, path[0]*-1);


        loading_title("Calculating poses");
        int offset = 0;
        int calcframes = N_FRAMES;
        loading_bar(0, calcframes);
        for (int newframe = 1; newframe < calcframes; newframe++) {
            loading_bar(newframe, calcframes);
            //cout << "New frame: " << newframe << endl;
            while (frames.size() > iterframes) {
                frames.erase(frames.begin());
                offset++;
            }
            loadZB(host_img, 512, 424, newframe);
            frames.push_back(dvo.createDepthImage(host_img));

            path.push_back(se3(path[path.size()-1].l));

            loadZB(host_img, 512, 424, newframe);
            DepthImage new_dest = dvo.createDepthImage(host_img);

            float ivar[iterframes];
            for (int i = 0; i < iterframes; i++) ivar[i] = 250;
            for (int scale = scales-1; scale >= 0; scale--) {
                //cout << "Next scale: " << scale << endl;
                float lasterror = 1e9;
                se3 lastview = path[newframe];

                for (int iter = 0; iter < 10; iter++) {

                    int iterframe = 0;
                    /*std::priority_queue<std::pair<int, int> > pq;
        for (int frame = 0; frame < newframe; frame++) {
        pq.push(std::make_pair(overlap(path[frame], path[newframe]), frame));
        }
        while (pq.size() && iterframe < iterframes) {
        int frame = pq.top().second;
        pq.pop();*/

                    for (int frame = std::max(0, newframe-iterframes); frame < newframe; frame++) {
                        se3 diff = path[newframe]*(path[frame]*-1);

                        //for (int i = 0; i < 6; i++) cout << diff.l[i] << ' ';
                        //cout << endl;
                        diff.exp(transform,0);

                        queue.enqueueWriteBuffer(dvo.device_transform, 0, 0, sizeof(float)*12, transform);
                        dvo.calcGaussNewton(frames[frame-offset], new_dest, scale, Ab+iterframe*29*2, ivar[iterframe]);
                        (diff*-1).exp(transform,0);
                        queue.enqueueWriteBuffer(dvo.device_transform, 0, 0, sizeof(float)*12, transform);
                        dvo.calcGaussNewton(new_dest, frames[frame-offset], scale, Ab+iterframe*29*2+29, ivar[iterframe]);

                        iterframe++;
                    }
                    queue.finish();


                    //for (int i = 0; i < iterframe; i++) {
                    //  ivar[i] = Ab[i*29+27]/Ab[i*29+28];
                    //}
                    for (int frame = 1; frame < iterframe*2; frame++)
                        for (int i = 27; i < 29; i++)
                            Ab[i] += Ab[frame*29+i];

                    //for (int i = 0; i < iterframe; i++) {
                    //  ivar[i] = Ab[28]/Ab[27];
                    //}

                    /*for (int i = 0; i < 29; i++)
        cout << Ab[i] << ' ';
        cout << endl;*/
                    float error = Ab[28]/Ab[27];
                    //cout << Ab[27] << ' ' << Ab[28] << endl;
                    //if (scale == 2)
                    //cout << scale << ": " << 1./error << " " << ivar[iterframe-1] << endl;
                    //ivar = 1./error;
                    //cout << 1e6*error << ' ' << 1./error << ' ' << Ab[27] << endl;
                    if (error != error or error > lasterror*.99) {
                        path[newframe] = lastview;
                        break;
                    }
                    lastview = se3(path[newframe].l);
                    lasterror = error;

                    iterframe = 0;
                    float tot_A[36], tot_b[6], x[6] = {0}, tmp1[6], tmp2[6];
                    for (int i = 0; i < 36; i++) tot_A[i] = 0;
                    for (int i = 0; i < 6; i++) tot_b[i] = 0;
                    for (int frame = std::max(0, newframe-iterframes); frame < newframe; frame++) {
                        float A[36*2], b[6*2];
                        se3 diff = path[newframe]*(path[frame]*-1);
                        for (int k = 0; k < 2; k++) {
                            int c = 0;
                            float*pAb = Ab+iterframe*29*2;
                            for (int j = 0; j < 6; j++)
                                for (int i = 0; i <= j; i++)
                                    A[i+j*6+36*k] = pAb[29*k+c++];
                            for (int j = 0; j < 6; j++)
                                for (int i = j+1; i < 6; i++)
                                    A[i+j*6+36*k] = A[j+i*6+36*k];
                            for (int i = 0; i < 6; i++) b[6*k+i] = -pAb[29*k+c++];
                        }
                        fix(A+36, b+6, diff*-1);
                        for (int k = 0; k < 2; k++) {
                            for (int i = 0; i < 36; i++) tot_A[i] += A[i+36*k];
                            for (int i = 0; i < 6; i++) tot_b[i] += b[i+6*k];
                        }
                        iterframe++;
                    }

                    solve(tot_A, tot_b, x, tmp1, tmp2, 6);
                    //for (int i = 0; i < 6; i++) cout << x[i] << ' ';
                    //cout << endl;

                    se3 updateView(x);
                    path[newframe] = updateView*path[newframe];
                    //cout << Ab[27] << ' ' << Ab[28] << endl;
                    //for (int i = 0; i < 29; i++) cout << Ab[i] << endl;

                    /*for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++)
        cout << setw(8) << tA[i*6+j] << ' ';
        cout << ' ' << setw(8) << x[i+6*iterframes] << endl;
        }
        cout << endl;*/
                }
            }
            dumpPose(tmp_fp, path[newframe]*-1);
        }
        fclose(tmp_fp);
        loading_finish(calcframes);

        FILE*fp = fopen(VIEW, "w");
        for (int i = 0; i < calcframes; i++) {
            se3 view = path[i]*-1;
            view.exp(transform, 0);
            //if (i)
            //cout << var[i]-var[i-1] << endl;
            //fprintf(fp, "%f %f %f", transform[3], transform[7], transform[11]);
            for (int k = 0; k < 3; k++) {
                for (int j = 0; j < 3; j++)
                    fprintf(fp, " %f", transform[j+k*4]);
                fprintf(fp, " %f\n", transform[3+k*4]);
            }
            fprintf(fp, " %f %f %f %f\n\n", 0.0, 0.0, 0.0, 1.0);
        }
        fclose(fp);

    } catch (cl::Error e) {
        cout << endl << e.what() << " : " << errlist[-e.err()] << endl;
    }
}
