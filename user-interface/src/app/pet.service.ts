import { Injectable } from '@angular/core';

@Injectable({
  providedIn: 'root',
})
export class PetService {
  constructor() {}

  getPetPosition(): { lat: number; lng: number; timestamp: number } {
    return {
      lat: 41.8941 + (Math.random() - 0.5) / 100,
      lng: 12.495 + (Math.random() - 0.5) / 100,
      timestamp: Date.now() - 10000,
    };
  }

  getGeofence(): [number, number][] {
    return [
      [41.89615593693024, 12.48712220789482],
      [41.90194818191196, 12.497284633845346],
      [41.89046959691638, 12.49905267353532],
      [41.88567734430687, 12.489890247584794],
      [41.89136368987694, 12.477959781944294],
    ];
  }
}
