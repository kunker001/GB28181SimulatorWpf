using System.Windows;
using System.Windows.Media;

namespace GB28181SimulatorWpf
{
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            // Force software rendering to avoid GPU issues on VMs/headless servers
            RenderOptions.ProcessRenderMode = System.Windows.Interop.RenderMode.SoftwareOnly;
            base.OnStartup(e);
        }
    }
}
