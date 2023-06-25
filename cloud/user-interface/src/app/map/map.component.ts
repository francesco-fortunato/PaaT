import { Component } from '@angular/core';
import * as L from 'leaflet';
import { PetService } from '../pet.service';
import '@geoman-io/leaflet-geoman-free';
import 'leaflet-draw';
import 'leaflet-draw-drag';

@Component({
  selector: 'app-map',
  templateUrl: './map.component.html',
  styleUrls: ['./map.component.css'],
})
export class MapComponent {
  private map: L.DrawMap | undefined;
  gps: L.Marker | undefined;
  pet: L.Marker | undefined;
  // private geoFence: L.Polygon<any> | undefined;
  private geoFence: L.Rectangle | undefined;
  private gpsIcon = L.icon({
    iconUrl: 'assets/gps.png',
    iconSize: [40, 40],
    iconAnchor: [20, 20],
  });
  private petIcon = L.icon({
    iconUrl: 'assets/pet.png',
    iconSize: [40, 40],
    iconAnchor: [20, 20],
  });
  editingFence: boolean = false;
  // fenceMenuOpen: boolean = false;
  lightOn: boolean = false;
  soundOn: boolean = false;
  timelineOpen: boolean = false;
  petStatus:
    | {
        Light: string;
        Sound: string;
        sample_time: number;
        Longitude: string;
        Latitude: string;
        device_id: number;
      }
    | undefined;
  timeline: L.LayerGroup | undefined;
  lightEnabled: boolean = false;
  soundEnabled: boolean = false;
  fenceEnabled: boolean = true;
  lightSoundToggleTimeout: any;

  constructor(private petService: PetService) {}

  private initMap(): void {
    this.map = L.map('map', {
      center: [41.891106645704795, 12.503608718298109],
      zoom: 18,
      attributionControl: false,
      zoomControl: false,
    });

    L.control
      .zoom({
        position: 'topright',
      })
      .addTo(this.map);

    const tiles = L.tileLayer(
      'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
      {
        maxZoom: 19,
        minZoom: 5,
      }
    );

    tiles.addTo(this.map);
    this.petService.getGeofence(1).subscribe((res) => {
      if (res) {
        // this.geoFence = L.polygon(res.device_data[0], {
        //   fillOpacity: 0.3,
        // });
        this.geoFence = L.rectangle(L.latLngBounds(res.device_data[0]), {
          fillOpacity: 0.3,
        });
        if (this.map) {
          this.geoFence.addTo(this.map);
        }
      }
      if (!this.checkFence()) {
        this.lightEnabled = true;
        this.soundEnabled = true;
      }
    });
    // this.map.on('pm:create', (e) => {
    //   if (this.map) {
    //     if (e.layer instanceof L.Polygon) {
    //       this.geoFence = e.layer;
    //       this.geoFence.addTo(this.map);
    //       this.editingFence = false;
    //       this.petService
    //         .saveGeofence(1, this.geoFence.getLatLngs())
    //         .subscribe({
    //           next: () => {},
    //           error: (e) => console.error(e),
    //           complete: () => console.info('complete'),
    //         });
    //     }
    //   }
    // });
    this.map.on('draw:created', (e) => {
      if (this.map) {
        this.geoFence = e.layer;
        if (this.geoFence) {
          console.log(this.geoFence.getLatLngs());
          this.geoFence.addTo(this.map);
          this.editingFence = false;
          this.petService
            .saveGeofence(1, this.geoFence.getLatLngs())
            .subscribe({
              next: () => {},
              error: (e) => console.error(e),
              complete: () => console.info('complete'),
            });
        }
      }
    });
  }

  ngAfterViewInit(): void {
    this.initMap();
    if (this.map) {
      this.initPet();

      this.initGps();
      this.trackPet();
      this.trackGps();
      // this.lightOn =  this.petService.lightStatus();
      // this.soundOn = this.petService.soundStatus();
    }
  }

  initGps(): void {
    if (this.map) {
      this.map.locate({ watch: true, enableHighAccuracy: true });
      this.map.on('locationfound', (e) => {
        if (!this.gps && this.map) {
          this.gps = L.marker(e.latlng, {
            icon: this.gpsIcon,
          });
          this.gps.addTo(this.map);
        }
      });
    }
  }

  initPet(): void {
    this.petService.getPetStatus(1).subscribe((res) => {
      this.petStatus = res;
      if (this.map) {
        this.pet = L.marker(
          L.latLng(+this.petStatus.Latitude, +this.petStatus.Longitude),
          {
            icon: this.petIcon,
          }
        );
        this.pet.bindPopup(
          'Last position update: ' +
            new Date(this.petStatus.sample_time).toLocaleString('it')
        );
        this.pet.addTo(this.map);
      }
      this.gotoPet();
    });
  }

  gotoGps(): void {
    if (this.map && this.gps) {
      this.map.flyTo(this.gps.getLatLng(), 18);
    } else {
      alert(
        'PaaT cannot access your location. Please turn on GPS and check your browser settings.'
      );
    }
  }

  gotoPet(): void {
    if (this.map && this.pet) {
      this.map.flyTo(this.pet.getLatLng(), 18);
    } else {
      alert('PaaT cannot access actual Pet location.');
    }
  }

  toggleLight(): void {
    if (this.lightEnabled) {
      if (!this.lightOn) {
        this.lightOn = true;
      } else {
        this.lightOn = false;
      }
      if (!this.lightSoundToggleTimeout) {
        this.lightSoundToggleTimeout = setTimeout(() => {
          this.petService
            .setLightOrSound(1, this.lightOn, this.soundOn)
            .subscribe({
              next: () => {},
              error: (e) => {},
              complete: () => console.info('complete'),
            });
          this.lightSoundToggleTimeout = undefined;
        }, 60000);
      }
    } else {
      this.lightOn = false;
    }
  }

  toggleSound(): void {
    if (this.soundEnabled) {
      if (!this.soundOn) {
        this.soundOn = true;
      } else {
        this.soundOn = false;
      }
      if (!this.lightSoundToggleTimeout) {
        this.lightSoundToggleTimeout = setTimeout(() => {
          this.petService
            .setLightOrSound(1, this.lightOn, this.soundOn)
            .subscribe({
              next: () => {},
              error: (e) => {},
              complete: () => console.info('complete'),
            });
          this.lightSoundToggleTimeout = undefined;
        }, 60000);
      }
    } else {
      this.soundOn = false;
    }
  }

  toggleTimeline(): void {
    if (this.map) {
      if (this.timeline && this.timelineOpen) {
        this.timelineOpen = false;
        this.map.removeLayer(this.timeline);
        this.timeline = undefined;
      } else {
        this.petService.getLatestPetPath(1).subscribe(
          (
            res: {
              device_id: number;
              sample_time: number;
              Longitude: number;
              Latitude: number;
              Satellites: number;
            }[]
          ) => {
            const response = res.sort((a, b) => a.sample_time - b.sample_time);
            const timelinePositions: L.LatLng[] = response.map(
              (position: {
                device_id: number;
                sample_time: number;
                Longitude: number;
                Latitude: number;
                Satellites: number;
              }) => {
                return L.latLng([position.Latitude, position.Longitude]);
              }
            );

            const timelineTrack = L.polyline(timelinePositions, {
              fillOpacity: 0.3,
              color: 'red',
            });
            const timelineMarkers = response.map((position, index, pos) => {
              return L.marker(timelinePositions[index], {
                icon: L.divIcon({
                  className:
                    index === 0
                      ? 'timeline-start'
                      : index === pos.length - 1
                      ? 'timeline-stop'
                      : 'timeline-marker',
                  html: '<div></div>',
                }),
              }).bindPopup(new Date(position.sample_time).toLocaleString('it'));
            });

            this.timeline = L.layerGroup([timelineTrack, ...timelineMarkers]);
            if (this.map) {
              this.timeline.addTo(this.map);
            }
            this.timelineOpen = true;
          }
        );
      }
    }
  }

  // toggleFenceOptions(): void {
  //   if (this.map) {
  //     if (!this.geoFence) {
  //       if (this.editingFence) {
  //         this.editingFence = false;
  //         this.map.pm.disableGlobalEditMode();
  //       } else {
  //         // this.fenceMenuOpen = false;
  //         this.editingFence = true;
  //         this.map.pm.enableDraw('Polygon', {});
  //       }
  //     } else {
  //       if (this.editingFence) {
  //         this.map.pm.disableGlobalEditMode();
  //         this.editingFence = false;
  //         const latLng = this.geoFence.getLatLngs();
  //         // this.petService.saveGeofence(this.geoFence.getLatLngs());
  //       } else if (this.fenceMenuOpen) {
  //         // this.fenceMenuOpen = false;
  //       } else {
  //         // this.fenceMenuOpen = true;
  //       }
  //     }
  //   }
  // }

  toggleFenceOptions(): void {
    if (this.map && this.fenceEnabled) {
      if (!this.geoFence) {
        this.editingFence = true;
        // this.map.pm.enableDraw('Polygon', {});
        new L.Draw.Rectangle(this.map).enable();
      } else {
        if (this.editingFence) {
          this.map.pm.disableGlobalEditMode();
          this.editingFence = false;
          this.petService
            .saveGeofence(1, this.geoFence.getLatLngs())
            .subscribe({
              next: () => {
                alert(
                  'Geofence saved! You must now reboot the pet tracker manually.'
                );
              },
              error: (e) => console.error(e),
              complete: () => console.info('complete'),
            });
          this.lightEnabled = false;
          this.soundEnabled = false;
          this.fenceEnabled = false;
        } else {
          this.editFence();
        }
      }
    }
  }

  editFence(): void {
    // this.fenceMenuOpen = false;
    this.editingFence = true;
    if (this.map && this.geoFence) {
      this.geoFence.pm.enable();
    }
  }

  removeFence(): void {
    // this.fenceMenuOpen = false;
    if (this.map && this.geoFence) {
      this.map.removeLayer(this.geoFence);
      this.geoFence = undefined;
      // this.petService.removeGeofence(1);
    }
  }

  // update periodically the pet position
  trackPet(): void {
    setInterval(() => {
      this.petService.getPetStatus(1).subscribe((res) => {
        this.petStatus = res;
        if (this.map) {
          if (!this.pet) {
            this.pet = L.marker(
              L.latLng(+this.petStatus.Latitude, +this.petStatus.Longitude),
              {
                icon: this.petIcon,
              }
            );
            this.pet.addTo(this.map);
            this.gotoPet();
          } else {
            this.pet.setLatLng(
              L.latLng(+this.petStatus.Latitude, +this.petStatus.Longitude)
            );
          }
          this.pet.bindPopup(
            'Last position update: ' +
              new Date(this.petStatus.sample_time).toLocaleString('it')
          );
        }
        this.lightOn = this.petStatus.Light === 'true';
        this.soundOn = this.petStatus.Sound === 'true';

        this.fenceEnabled = true;
        if (this.geoFence && this.checkFence()) {
          this.lightEnabled = false;
          this.soundEnabled = false;
        } else {
          this.lightEnabled = true;
          this.soundEnabled = true;
        }
      });
    }, 60000);
  }
  //update periodically the gps position
  trackGps(): void {
    setInterval(() => {
      if (this.map) {
        this.map.locate({ watch: true, enableHighAccuracy: true });
        this.map.on('locationfound', (e) => {
          if (this.gps) {
            this.gps.setLatLng(e.latlng);
          }
        });
        this.map.once('locationerror', (e) => {
          this.gps = undefined;
        });
      }
    }, 10000);
  }

  // check if pet is inside the fence
  checkFence(): boolean {
    if (this.geoFence && this.pet) {
      return this.geoFence.getBounds().contains(this.pet.getLatLng());
    }
    return false;
  }
}
