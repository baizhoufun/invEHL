#include <iostream>
#include <eigen3/Eigen/Core>
#include <thread>

#include "io/ioEigen.hpp"
#include "io/utilities.hpp"
#include "pde/tfe.hpp"

using namespace cv;

class ehl : public invEHL::pde::TFE
{
public:
    explicit ehl(const std::string &iniFIleName) : invEHL::pde::TFE(iniFIleName) {}
    static double setMaskFunc(void *context, double x, double y) { return static_cast<ehl *>(context)->maskFunc(x, y); }

private:
    double maskFunc(double x, double y)
    {
        double kx = 2.0 * M_PI / mesh.info.lx;
        double ky = 2.0 * M_PI / mesh.info.ly;
        return 0.3 * cos(kx * x);
    };
};

int main(int argc, char **argv)
{
    std::string confPath = "../resources/in.ini";
    invEHL::io::INIReader reader(confPath);

    if (reader.ParseError() < 0)
    {
        std::cout << "Can't load 'test.ini'\n";
        return 1;
    };

    if (argc > 1)
        confPath = argv[1];

    //invEHL::io::Utilities::waterMark();

    //invEHL::pde::TFE tfe(confPath);
    ehl tfe(confPath);
    tfe.rescale(0.5, 0.0, tfe.data.control());

    //tfe.mesh.initVector(ehl::setMaskFunc, &tfe, tfe.data.control());

    invEHL::io::IOEigen::write("../resources/control.txt", tfe.data.control());
    tfe.setFunction(tfe.data.state()[0], 0.15);
    tfe.data.time()[0] = 0;

    double dt = tfe.param.dt;
    double dtLast = dt;
    double tNow = 0;
    int q = 0;
    double g = 0.1;
    Eigen::VectorXd nextState;
    nextState.resize(tfe.mesh.info.dof);
    nextState.setZero();

    while (tfe.BDF(tfe.data.state()[0], tfe.data.state()[0], nextState, dt, dt, 1, invEHL::pde::TFE::Flag::BDFINFO_ON) != 0)
    {
        q = q + 1;
        dt = dt / pow(1.0 + g, q);
        std::cout << "REDUCE TIME STEP ...\n";
        std::cout << tNow << " | " << dtLast << " | " << dt;
    }
    tNow += dt;
    tfe.data.state().push_back(nextState);
    tfe.data.time().push_back(tNow);

    int i = 2;
    while (tNow < tfe.param.tMax)
    //for (int i = 2; i < tfe.param.tStep; ++i)
    {
        const Eigen::VectorXd &stateLast = tfe.data.state()[i - 1];
        invEHL::io::IOEigen::mat2Img(stateLast,
                                     tfe.mesh.info.ny * 2, tfe.mesh.info.nx * 2, tfe.mesh.info.ly / tfe.mesh.info.lx,
                                     0, 0.5);
        std::cout << tNow << " | " << dtLast << " | " << dt;
        q = 0;

        if (dt > tfe.param.tMax - tNow)
        {
            dt = tfe.param.tMax - tNow;
        }
        while (tfe.BDF(tfe.data.state()[i - 2], tfe.data.state()[i - 1], nextState, dtLast, dt, tfe.param.bdf, invEHL::pde::TFE::Flag::BDFINFO_ON) != 0)
        {
            q = q + 1;
            dt = dt / pow(1.0 + g, q);
            std::cout << "REDUCE TIME STEP ...\n";
            std::cout << tNow << " | " << dtLast << " | " << dt;
            //tfe.BDF(tfe.data.state()[i - 2], tfe.data.state()[i - 1], tfe.data.state()[i], dtLast, dt, 2, invEHL::pde::TFE::Flag::BDFINFO_ON);
        }
        tNow += dt;
        dtLast = dt;
        i++;
        tfe.data.state().push_back(nextState);
        tfe.data.time().push_back(tNow);
        //tfe.BDF(tfe.data.state()[i - 1], tfe.data.state()[i - 1], tfe.data.state()[i], tfe.param.dt, tfe.param.dt, 1, invEHL::pde::TFE::Flag::BDFINFO_ON);
    }
    const Eigen::VectorXd &stateLast = tfe.data.state()[i - 1];
    invEHL::io::IOEigen::mat2Img(stateLast,
                                 tfe.mesh.info.ny * 2, tfe.mesh.info.nx * 2, tfe.mesh.info.ly / tfe.mesh.info.lx,
                                 0, 0.5);
    std::cout << tNow << " | " << dtLast << " | " << dt;

    invEHL::io::IOEigen::write("../resources/state/", tfe.data.state());
    invEHL::io::IOEigen::write("../resources/mesh/time.txt", invEHL::io::IOEigen::std2Eigenvec(tfe.data.time()));
    tfe.postProcess("../resources/misc/output.txt");

    return 0;
}
