#include "gfx/gfx_objects.h"

class LittleRendererWindow final : public LittleGFXWindow
{
public:
    void Run() override
    {
        MSG msg = { 0 };
        while (msg.message != WM_QUIT)
        {
            // ����ϵͳ����Ϣ
            if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            // �ڿ���ʱ���������Լ����߼�
            else
            {
                // ��ʱʲô������
                // Sleep 1~2ms �����������̱߳�while��ռ
                Sleep(1);
            }
        }
        // ����յ���WM_QUIT��Ϣ������ֱ���˳��˺���
        return;
    }
};

int main(void)
{
    // ��������ʼ��ʵ��
    auto instance = LittleFactory::Create<LittleGFXInstance>(true);
    auto device = LittleFactory::Create<LittleGFXDevice>(instance->GetAdapter(0));
    // ��������ʼ��������
    auto window = LittleFactory::Create<LittleRendererWindow>(L"BlueRoseRender", device, true);
    // ���д������ѭ��
    window->Run();
    // ���ڴ����Ѿ��رգ�������������
    LittleFactory::Destroy(window);
    // ����ʵ��
    LittleFactory::Destroy(device);
    LittleFactory::Destroy(instance);

    return 0;
}